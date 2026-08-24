#include "LitObjectStore.h"
#include "LitBlob.h"
#include "LitRepository.h"

#include <openssl/sha.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

using std::filesystem::path;

/*
  Since C++ doesn't have built in hashing like SHA1 and z compressing,
  we have to import and link libraries, and use them to write similar
  functionality. 
  [Warning: Sha1 is considered unsafe, consider upgrading to higher standard]
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

}

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

  // Hash uncompressed git object's represendation (<type> <size>\0<payload>)
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
  
}



