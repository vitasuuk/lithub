#include "LitObjectStore.h"
#include "LitBlob.h"
#include "LitCommit.h"
#include "LitTag.h"
#include "LitTree.h"
#include "LitRepository.h"
#include "LitRefs.h"

#include <openssl/sha.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

using std::filesystem::path;

/*
  Since C++ doesn't have built in hashing like SHA1 and z compressing,
  we have to import and link libraries, and use them to write similar
  functionality. 
  
  Lit currently follows Git's SHA1 object format. OpenSSL's low-level
  SHA1_Init/Update/Final API is deprecated; can later be replaced with
  the EVP API while still computing SHA1.
*/
namespace {

// Compute SHA1 hash of passed data and return it as 40hex-character
// lowercase hexadecimal string.
std::string ComputeSha1(const bytes& data) {
  unsigned char hash[SHA_DIGEST_LENGTH]; // 20 bytes
  SHA_CTX sha_ctx;

  // Initialize SHA-1 context
  if (!SHA1_Init(&sha_ctx)) {
      throw std::runtime_error("SHA1_Init failed");
  }

  // Update with input data
  if (!SHA1_Update(&sha_ctx, data.data(), data.size())) {
    throw std::runtime_error("SHA1_Update failed");
  }

  // Finalize and get the hash
  if (!SHA1_Final(hash, &sha_ctx)) {
    throw std::runtime_error("SHA1_Final failed");
  }

  // Convert to hex string
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
    oss << std::setw(2) << static_cast<unsigned int>(hash[i]);
  }

  return oss.str(); // 40-character hex string
}

bytes Compress(const bytes& data) {
  // Find an upper bound for potential size of compressed item.
  uLongf compressed_size = compressBound(static_cast<uLong>(data.size()));
  bytes compressed(compressed_size);

  int result = compress2(
      reinterpret_cast<Bytef*>(compressed.data()),
      &compressed_size,
      reinterpret_cast<const Bytef*>(data.data()),
      static_cast<uLong>(data.size()),
      Z_DEFAULT_COMPRESSION);

  if (result != Z_OK) {
    throw std::runtime_error("Failed to compress object.");
  }

  compressed.resize(compressed_size);  // shrink the vector to actual size
  return compressed;
}

bytes Decompress(const bytes& data) {
  // Unlike with compress, we don't know the size the given item will
  // decompress to. Therefore, we keep decompressing by chunks.
  z_stream stream{};

  stream.next_in = const_cast<Bytef*>(
      reinterpret_cast<const Bytef*>(data.data()));
  
  stream.avail_in = static_cast<uInt>(data.size());

  if (inflateInit(&stream) != Z_OK) {
    throw std::runtime_error("Failed to initialize decompression.");
  }

  bytes result;
  std::array<std::uint8_t, 4096> buff;
  int status = Z_OK;

  while (status != Z_STREAM_END) {
    stream.next_out = reinterpret_cast<Bytef*>(buff.data());
    stream.avail_out = buff.size();

    status = inflate(&stream, Z_NO_FLUSH);

    if (status != Z_OK && status != Z_STREAM_END) {
      inflateEnd(&stream);
      throw std::runtime_error("Failed to decompress object.");
    }

	std::size_t bytes_written = buff.size() - stream.avail_out;

	result.insert(result.end(), buff.begin(), buff.begin() + bytes_written);
  }

  inflateEnd(&stream);

  return result;
}

}  // namespace

std::string ObjectWrite(const LitObject& object, LitRepository* repo) {
  bytes data = object.Serialize();
  // Lit Header format  <type> <size>\0<payload>
  std::string header = std::string(object.Format()) + " " +
                       std::to_string(data.size());
  header.push_back('\0');
  
  bytes result;
  result.reserve(header.size() + data.size());  // for preventing reallocation
  result.insert(result.end(), header.begin(), header.end());
  result.insert(result.end(), data.begin(), data.end());

  // Hash uncompressed lit object's represendation (<type> <size>\0<payload>)
  std::string sha = ComputeSha1(result);

  // if repo == null ptr, only hash without storing.
  if (repo == nullptr) {
    return sha;
  }

  path obj_path = repo->RepoFile(true, "objects", 
                                 sha.substr(0, 2), sha.substr(2));
  if (std::filesystem::exists(obj_path)) {
    return sha;
  }

  bytes compressed = Compress(result);
  
  std::ofstream file(obj_path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to create object file.");
  }

  file.write(reinterpret_cast<const char*>(compressed.data()),
      static_cast<std::streamsize>(compressed.size()));

  if (!file) {
    throw std::runtime_error("Failed to write object file.");
  }

  return sha;
}

std::unique_ptr<LitObject> ObjectRead(LitRepository& repo,
                                      const std::string& sha) {
  if (sha.size() != 40) {
    throw std::runtime_error("Invalid SHA1 hashing.");
  }

  path obj_path = repo.RepoPath("objects", sha.substr(0,2), sha.substr(2));

  if (!std::filesystem::is_regular_file(obj_path)) {
    return nullptr;
  }

  std::ifstream file(obj_path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open object: " + obj_path.string());
  }

  file.seekg(0, std::ios::end);
  std::streamsize file_size = file.tellg();

  file.seekg(0, std::ios::beg);
  if (file_size < 0) {
    throw std::runtime_error("Failed to determine obj file size: " + 
        obj_path.string());
  }

  // Read compressed data
  bytes compressed(static_cast<std::size_t>(file_size));
  
  if (file_size > 0) {
    file.read(reinterpret_cast<char*>(compressed.data()), file_size);

    if (!file) {
      throw std::runtime_error("Failed to read object: " + obj_path.string()); 
    }
  }

  // Decompress to internal object representation:
  bytes raw = Decompress(compressed);

  // iterators to construct the string later
  auto space = std::find(raw.begin(), raw.end(),
      static_cast<std::uint8_t>(' '));
  if (space == raw.end()) {
    throw std::runtime_error("Malformed object: " + sha +
        " has no format separator");
  }

  std::string format(raw.begin(), space);

  auto null_byte = std::find(space + 1, raw.end(), 
      static_cast<std::uint8_t>('\0'));
  if (null_byte == raw.end()) {
    throw std::runtime_error("Malformed object: " + sha +
        " has no null separator");
  }

  std::string size_field(space + 1, null_byte);
  std::size_t expected_size = std::stoull(size_field);

  bytes data(null_byte + 1, raw.end());

  if (expected_size != data.size()) {
    throw std::runtime_error("Malformed object: " + sha +
        " has unexpected length");
  }

  // Return the type of object according to the format
  if (format == "blob") {
    return std::make_unique<LitBlob>(data);
  } else if (format == "commit") {
	  return std::make_unique<LitCommit>(data);
  } else if (format == "tree") {
	  return std::make_unique<LitTree>(data);
  } else if (format == "tag") {
    return std::make_unique<LitTag>(data);
  }

  throw std::runtime_error("Unsupported object type: " + format);
}

// Checks the passed string if it fits sha format, used in object reference
// resolution to check if given by user name is already a SHA.
bool IsPossibleSha(const std::string& name) {
  if (name.size() < 4 || name.size() > 40) {
    return false;
  }

  for (char ch : name) {
    if (!std::isxdigit(static_cast<unsigned char>(ch))) {
      return false;
    }
  }

  return true;
} 

// Resolves name to an object hash in repo.
// This method is aware of:
// - short and long hashes
// - tags
// - branches
// - remote branches [?]
std::vector<std::string> ObjectResolve(LitRepository& repo,
                                       const std::string& name) {
  if (name.empty()) {
    return {};
  }

  if (name == "HEAD") {
    auto sha = RefResolve(repo, name);
    if (sha) {
      return {sha.value()};
    }
    return {};
  }

  std::vector<std::string> candidates;
  
  // Try if it is already sha (including partial)
  if (IsPossibleSha(name)) {
    std::string prefix = name.substr(0, 2);

    path possible_path = repo.RepoDir(false, "objects", prefix);
    if (!possible_path.empty()) {
      std::string remain = name.substr(2);
      for (auto& file : std::filesystem::directory_iterator(possible_path)) {
        std::string file_name = file.path().filename().string();

        if(file_name.rfind(remain, 0) == 0) {
          candidates.push_back(prefix + file_name);
        }
      }
    }
  }

  // Also check references
  auto check_tag = RefResolve(repo, "refs/tags/" + name);
  if (check_tag) {
    candidates.push_back(check_tag.value());
  }

  auto check_branch = RefResolve(repo, "refs/heads/" + name);
  if (check_branch) {
    candidates.push_back(check_branch.value());
  }

  auto check_remote = RefResolve(repo, "refs/remotes/" + name);
  if (check_remote) {
    candidates.push_back(check_remote.value());
  }
  
  return candidates;
}

std::optional<std::string> ObjectFind(LitRepository& repo, 
                                      const std::string& name,
                                      const std::string& format, 
                                      bool follow) {
  std::vector<std::string> sha_candidates = ObjectResolve(repo, name);
  
  if (sha_candidates.size() == 0) {
    throw std::runtime_error("No reference found: " + name);
  }

  if (sha_candidates.size() > 1) {
    std::ostringstream message;
    message << "Ambiguous reference " + name + ":\n";

    for(const std::string& cand : sha_candidates) {
      message << "  " << cand << '\n';
    }

    throw std::runtime_error(message.str());
  }

  std::string sha = sha_candidates[0];
  
  if (format.empty()) {
    return sha;
  }

  while(true) {
    std::unique_ptr<LitObject> obj = ObjectRead(repo, sha);

    if (!obj) {
      throw std::runtime_error("Object not found: " + sha);
    }

    if (obj->Format() == format) {
      return sha;
    }

    if (!follow) {
      return std::nullopt;
    }

    // Follow formats
    if (obj->Format() == "tag") {
      LitTag* tag_obj = dynamic_cast<LitTag*>(obj.get());

      sha = tag_obj->GetObjectSha();
    } else if (obj->Format() == "commit" && format == "tree") {
      LitCommit* commit_obj = dynamic_cast<LitCommit*>(obj.get());

      sha = commit_obj->GetTreeSha();
    } else {
      return std::nullopt;
    }
  }

}

std::string ObjectHash(std::istream& input, const std::string& format,
                       LitRepository* repo) {
  bytes data;

  char byte;
  while(input.get(byte)) {
    data.push_back(static_cast<std::uint8_t>(byte));
  }

  if (format == "blob") {
    LitBlob obj(data);
    return ObjectWrite(obj, repo);
  }

  throw std::runtime_error("Unknown object type: " + format);
}
