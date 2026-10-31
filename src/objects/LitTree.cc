#include "objects/LitTree.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Private Helper Methods
namespace {

  bytes HexToBytes(const std::string& hex) {
    if (hex.size() != 40) {
      throw std::runtime_error("Invalid SHA1.");
    }

    bytes result;
    result.reserve(20);

    for (std::size_t i = 0; i < hex.size(); i += 2) {
      std::string hex_form = hex.substr(i, 2);

      auto byte_form = static_cast<std::uint8_t>(
          std::stoul(hex_form, nullptr, 16));
      
      result.push_back(byte_form);
    }

    return result;
  }

  std::string BytesToHex(bytes::const_iterator begin,
                         bytes::const_iterator end) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (auto it = begin; it != end; ++it) {
      oss << std::setw(2)
          << static_cast<unsigned int>(*it);
    }

    return oss.str();
  }

  TreeEntry ParseEntry(const bytes& data, std::size_t& pos) {
    TreeEntry entry;

    auto space_pos = std::find(data.begin() + pos, data.end(),
        static_cast<std::uint8_t>(' '));

    entry.mode = std::string(data.begin() + pos, space_pos);

    // Git writes directory mode as "40000", normalize to 6 chars so
    // the rest of lithub can compare against "040000".
    if (entry.mode.size() == 5) {
      entry.mode = "0" + entry.mode;
    }

    auto null_pos = std::find(data.begin() + pos, data.end(),
        static_cast<std::uint8_t>('\0'));

    entry.path = std::string(space_pos + 1, null_pos);

    auto sha_end = null_pos + 21;

    entry.sha = BytesToHex(null_pos + 1, sha_end);

    pos = static_cast<std::size_t>(sha_end - data.begin());
    return entry;
  }

  // Helpers for comparison to preserve cannonical git tree elements ordering

  // If this entry's path is directory, return it with added '/' to
  // ensure specific ordering of files and directories.
  std::string TreeAdjustDirPath(const TreeEntry& entry) {
    if (entry.mode == "040000") {
      return entry.path + '/';
    }

    return entry.path;
  }

  bool CompareTreePaths(const TreeEntry& a, const TreeEntry& b) {
    return TreeAdjustDirPath(a) < TreeAdjustDirPath(b);
  }

}

LitTree::LitTree(const bytes& data) {
  Deserialize(data);
}

bytes LitTree::Serialize() const {
  // No matter what order entries_ currently has, convert it
  // to cannonical Git order first, to avoid different serialization
  // of same / similarly stractured trees and therefore different SHA hashing.
  std::vector<TreeEntry> sorted_entries = entries_;

  std::sort(sorted_entries.begin(), sorted_entries.end(), CompareTreePaths);

  bytes result;

  for (const TreeEntry& entry : sorted_entries) {
    result.insert(result.end(), entry.mode.begin(), entry.mode.end());
    result.push_back(' ');

    result.insert(result.end(), entry.path.begin(), entry.path.end());
    result.push_back('\0');

    bytes byte_sha = HexToBytes(entry.sha);
    result.insert(result.end(), byte_sha.begin(), byte_sha.end());
  }

  return result;
}

void LitTree::Deserialize(const bytes& data) {
  entries_.clear();
  std::size_t pos = 0;
  std::size_t end = data.size();

  while (pos < end) {
    entries_.push_back(ParseEntry(data, pos)); // decodes from bytes to hex
  }
}

std::string_view LitTree::Format() const {
  return "tree";
}
