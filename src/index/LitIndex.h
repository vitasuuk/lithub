#ifndef LIT_INDEX_H_
#define LIT_INDEX_H_

#include "repository/LitRepository.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using bytes = std::vector<std::uint8_t>;

struct LitIndexEntry {
  // The last time a file's metadata changed.  This is a pair
  // (timestamp in seconds, nanoseconds)
  std::pair<std::uint32_t, std::uint32_t> ctime;
  
  // The last time a file's data changed.  This is a pair
  // (timestamp in seconds, nanoseconds)
  std::pair<std::uint32_t, std::uint32_t> mtime;

  // The ID of device containing this file
  std::uint32_t dev;
  // The file's inode number
  std::uint32_t ino;

  // The object type, either b1000 (regular), b1010 (symlink),
  // b1110 (gitlink).
  std::uint16_t mode_type;
  // The object permissions
  std::uint16_t mode_perms;

  // User ID of owner
  std::uint32_t uid;
  // Group ID of owner
  std::uint32_t gid;
  // Size of this object, in bytes
  std::uint32_t file_size;
  // The object's sha
  std::string sha;

  bool flag_assume_valid;
  std::uint16_t flag_stage;

  // Name of the object (full path this time!)
  std::string name;
};


class LitIndex {
 public:
  LitIndex() = default;
  explicit LitIndex(const bytes& data);

  std::uint16_t GetVersion() const;

  const std::vector<LitIndexEntry>& GetEntries() const;

  // Used by rm/add to modify the staging area.
  std::vector<LitIndexEntry>& GetMutableEntries();

  void Deserialize(const bytes& data);

  // Inverse of Deserialize: header, then every entry padded to a multiple
  // of 8 bytes.
  // Note: real git also appends a SHA1 checksum of the whole file,
  // lithub (like wyag) neither writes nor verifies it.
  const bytes Serialize() const;

 private:
  std::uint32_t version_ = 2;
  std::vector<LitIndexEntry> entries_;
};

LitIndex IndexRead(LitRepository& repo);

void IndexWrite(LitRepository& repo, const LitIndex& index);

// Builds an index entry for a regular file from its filesystem metadata
// (stat). name is the path relative to the worktree root ("src/main.cc").
// POSIX only (relies on stat's nanosecond timestamps).
LitIndexEntry IndexEntryFromFile(const std::filesystem::path& file_path,
                                 const std::string& name,
                                 const std::string& sha);

#endif  // LIT_INDEX_H_
