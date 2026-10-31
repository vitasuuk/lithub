#include "index/LitIndex.h"

#include <sys/stat.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
// Github index disk format specifically uses big-endian representation.
// Can't use memcpy, since it interprets data according to native configuration.
// Therefore, manual interpreting is needed.


std::uint32_t ReadUint32BE(const bytes& data, std::size_t offset) {
  if (offset + 4 > data.size()) {
    throw std::runtime_error("Unexpected end of index file.");
  }

  return (static_cast<std::uint32_t>(data[offset]) << 24) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
         static_cast<std::uint32_t>(data[offset + 3]);
}

std::uint16_t ReadUint16BE(const bytes& data, std::size_t offset) {
  if (offset + 2 > data.size()) {
    throw std::runtime_error("Unexpected end of index file.");
  }

  return (static_cast<std::uint16_t>(data[offset]) << 8) |
         static_cast<std::uint16_t>(data[offset + 1]);
}

std::string BytesToHex(const bytes& data,
                       std::size_t offset,
                       std::size_t length) {
  if (offset + length > data.size()) {
    throw std::runtime_error("Unexpected end of index file.");
  }

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (std::size_t i = 0; i < length; ++i) {
    oss << std::setw(2)
        << static_cast<unsigned int>(data[offset + i]);
  }

  return oss.str();
}

// Writers mirror the readers above: big-endian, most significant byte first.
void WriteUint32BE(bytes& data, std::uint32_t value) {
  data.push_back(static_cast<std::uint8_t>(value >> 24));
  data.push_back(static_cast<std::uint8_t>(value >> 16));
  data.push_back(static_cast<std::uint8_t>(value >> 8));
  data.push_back(static_cast<std::uint8_t>(value));
}

void WriteUint16BE(bytes& data, std::uint16_t value) {
  data.push_back(static_cast<std::uint8_t>(value >> 8));
  data.push_back(static_cast<std::uint8_t>(value));
}

// Appends 40-char hex SHA as its 20 raw bytes.
void WriteHexAsBytes(bytes& data, const std::string& hex) {
  if (hex.size() != 40) {
    throw std::runtime_error("Invalid SHA1 in index entry: " + hex);
  }

  for (std::size_t i = 0; i < hex.size(); i += 2) {
    data.push_back(static_cast<std::uint8_t>(
        std::stoul(hex.substr(i, 2), nullptr, 16)));
  }
}

}  // namespace


LitIndex::LitIndex(const bytes& data) {
  Deserialize(data);
}

std::uint16_t LitIndex::GetVersion() const {
  return version_;
}

const std::vector<LitIndexEntry>& LitIndex::GetEntries() const {
  return entries_;
}

std::vector<LitIndexEntry>& LitIndex::GetMutableEntries() {
  return entries_;
}

const bytes LitIndex::Serialize() const {
  bytes data = {'D', 'I', 'R', 'C'};

  WriteUint32BE(data, version_);
  WriteUint32BE(data, static_cast<std::uint32_t>(entries_.size()));

  for (const LitIndexEntry& entry : entries_) {
    std::size_t entry_start = data.size();

    WriteUint32BE(data, entry.ctime.first);
    WriteUint32BE(data, entry.ctime.second);
    WriteUint32BE(data, entry.mtime.first);
    WriteUint32BE(data, entry.mtime.second);

    WriteUint32BE(data, entry.dev);
    WriteUint32BE(data, entry.ino);

    // Mode takes 4 bytes, upper two are unused.
    WriteUint16BE(data, 0);
    WriteUint16BE(data, static_cast<std::uint16_t>(
        (entry.mode_type << 12) | entry.mode_perms));

    WriteUint32BE(data, entry.uid);
    WriteUint32BE(data, entry.gid);
    WriteUint32BE(data, entry.file_size);

    WriteHexAsBytes(data, entry.sha);

    // Merge back two flags and name length into the same two bytes.
    // flag_stage is already stored in its bit position (masked, not shifted).
    std::uint16_t name_length = entry.name.size() >= 0x0FFF
        ? 0x0FFF
        : static_cast<std::uint16_t>(entry.name.size());

    std::uint16_t flags = (entry.flag_assume_valid ? 0x8000 : 0) |
                          entry.flag_stage |
                          name_length;

    WriteUint16BE(data, flags);

    data.insert(data.end(), entry.name.begin(), entry.name.end());
    data.push_back('\0');

    // Same padding rule as in Deserialize.
    std::size_t entry_size = data.size() - entry_start;
    std::size_t padding = (8 - (entry_size % 8)) % 8;

    data.insert(data.end(), padding, 0);
  }

  return data;
}

void LitIndex::Deserialize(const bytes& data) {
  constexpr std::size_t kHeaderSize = 12;
  constexpr std::size_t kEntryFixedSize = 62;
  entries_.clear();

  if (data.size() < kHeaderSize) {
    throw std::runtime_error("Malformed Index File: too small.");
  }

  // Header "DIRC"
  if (data[0] != 'D' ||
      data[1] != 'I' ||
      data[2] != 'R' ||
      data[3] != 'C') {
    throw std::runtime_error("Invalid index signature");
  }

  version_ = ReadUint32BE(data, 4);
  if (version_ != 2) {
    throw std::runtime_error("Only version 2 is supported.");
  }

  std::uint32_t entry_count = ReadUint32BE(data, 8);

  size_t idx = kHeaderSize;

  for (std::uint32_t i = 0; i < entry_count; i++) {
    std::size_t entry_start = idx;

    if (idx + kEntryFixedSize > data.size()) {
      throw std::runtime_error("Unexpected index file end.");
    }

    LitIndexEntry entry;

    // Times
    entry.ctime.first = ReadUint32BE(data, idx);
    entry.ctime.second = ReadUint32BE(data, idx + 4);

    entry.mtime.first = ReadUint32BE(data, idx + 8);
    entry.mtime.second = ReadUint32BE(data, idx + 12);

    // Filesystem metadata.
    entry.dev = ReadUint32BE(data, idx + 16);
    entry.ino = ReadUint32BE(data, idx + 20);

    // Git stores mode as 4 bytes. For the subset we're parsing,
    // the upper two bytes should be zero.
    std::uint16_t unused = ReadUint16BE(data, idx + 24);
    if (unused != 0) {
      throw std::runtime_error("Invalid index mode field.");
    }
    
    std::uint16_t mode = ReadUint16BE(data, idx + 26);

    entry.mode_type = mode >> 12;

    if (entry.mode_type != 0b1000 &&
        entry.mode_type != 0b1010 &&
        entry.mode_type != 0b1110) {
      throw std::runtime_error("Unsupported index entry mode.");
    }

    entry.mode_perms = mode & 0b0000000111111111;

    entry.uid = ReadUint32BE(data, idx + 28);
    entry.gid = ReadUint32BE(data, idx + 32);

    entry.file_size = ReadUint32BE(data, idx + 36);

    // SHA-1 is stored as 20 raw bytes.
    entry.sha = BytesToHex(data, idx + 40, 20);

    // Flags.
    std::uint16_t flags = ReadUint16BE(data, idx + 60);
    entry.flag_assume_valid = (flags & 0x8000) != 0;
    bool extended = (flags & 0x4000) != 0;

    if (extended) {
      throw std::runtime_error(
          "Extended index entries are not supported.");
    }

    entry.flag_stage = flags & 0x3000;

    std::uint16_t name_length = flags & 0x0FFF;

    // Done with the 62-byte fixed section.
    idx += kEntryFixedSize;

    if (name_length < 0x0FFF) {
      if (idx + name_length >= data.size()) {
        throw std::runtime_error("Invalid index pathname length.");
      }

      if (data[idx + name_length] != '\0') {
        throw std::runtime_error("Index pathname is not null terminated.");
      }

      entry.name = std::string(
          data.begin() + idx,
          data.begin() + idx + name_length);

      idx += name_length + 1;

    } else {
      // 0xFFF means the pathname is at least 4095 bytes.
      if (idx + 0x0FFF >= data.size()) {
        throw std::runtime_error("Invalid long index pathname.");
      }

        auto name_end = std::find(data.begin() + idx + 0x0FFF,
                                  data.end(),
                                  static_cast<std::uint8_t>('\0'));

      if (name_end == data.end()) {
        throw std::runtime_error(
            "Index pathname is not null terminated.");
      }

      entry.name = std::string(data.begin() + idx, name_end);

      idx = static_cast<std::size_t>(name_end - data.begin()) + 1;
    }

    // Each entry is padded until its total encoded size
    // is a multiple of 8 bytes.
    std::size_t entry_size = idx - entry_start;

    // last % 8 in case no padding needed (to get 0)
    std::size_t padding = (8 - (entry_size % 8)) % 8;

    if (idx + padding > data.size()) {
      throw std::runtime_error("Invalid index entry padding.");
    }

    idx += padding;

    entries_.push_back(entry);
  }
}

LitIndex IndexRead(LitRepository& repo) {
  std::filesystem::path index_path = repo.RepoPath("index");

  if (!std::filesystem::exists(index_path)) {
    return LitIndex();
  }

  std::ifstream file(index_path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open index: " + index_path.string());
  }

  file.seekg(0, std::ios::end);
  std::streamsize file_size = file.tellg();

  if (file_size < 0) {
    throw std::runtime_error("Failed to determine obj file size: " + 
        index_path.string());
  }

  file.seekg(0, std::ios::beg);

  bytes index_data(static_cast<std::size_t>(file_size));

  if (file_size > 0) {
    file.read(reinterpret_cast<char*>(index_data.data()), file_size);

    if (!file) {
      throw std::runtime_error("Failed to read object: " + index_path.string()); 
    }
  }

  LitIndex index;
  index.Deserialize(index_data);

  return index;
}

void IndexWrite(LitRepository& repo, const LitIndex& index) {
  std::filesystem::path index_path = repo.RepoFile(true, "index");
  bytes data = index.Serialize();

  std::ofstream file(index_path, std::ios::binary | std::ios::trunc);
  if (!file) {
    throw std::runtime_error("Failed to open index: " + index_path.string());
  }

  file.write(reinterpret_cast<const char*>(data.data()),
      static_cast<std::streamsize>(data.size()));

  if (!file) {
    throw std::runtime_error("Failed to write index: " + index_path.string());
  }
}

LitIndexEntry IndexEntryFromFile(const std::filesystem::path& file_path,
                                 const std::string& name,
                                 const std::string& sha) {
  struct stat st{};

  if (::stat(file_path.c_str(), &st) != 0) {
    throw std::runtime_error("Failed to stat file: " + file_path.string());
  }

  LitIndexEntry entry;

  entry.ctime = {static_cast<std::uint32_t>(st.st_ctim.tv_sec),
                 static_cast<std::uint32_t>(st.st_ctim.tv_nsec)};
  entry.mtime = {static_cast<std::uint32_t>(st.st_mtim.tv_sec),
                 static_cast<std::uint32_t>(st.st_mtim.tv_nsec)};

  entry.dev = static_cast<std::uint32_t>(st.st_dev);
  entry.ino = static_cast<std::uint32_t>(st.st_ino);

  // Only regular files are supported for now. Git only distinguishes
  // executable (755) and non-executable (644) files.
  entry.mode_type = 0b1000;
  entry.mode_perms = (st.st_mode & S_IXUSR) ? 0755 : 0644;

  entry.uid = static_cast<std::uint32_t>(st.st_uid);
  entry.gid = static_cast<std::uint32_t>(st.st_gid);
  entry.file_size = static_cast<std::uint32_t>(st.st_size);

  entry.sha = sha;
  entry.flag_assume_valid = false;
  entry.flag_stage = 0;
  entry.name = name;

  return entry;
}
