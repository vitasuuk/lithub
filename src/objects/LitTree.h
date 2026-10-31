#ifndef LIT_TREE_H_
#define LIT_TREE_H_

#include "objects/LitObject.h"

#include <string>
#include <string_view>
#include <vector>

// Container for one entry of tree object
struct TreeEntry {
  std::string mode;
  std::string path;
  std::string sha;
};

// Represents a full tree object.
// A tree contains the immediate files/directories of one directory.
// Each entry points to either a blob or another tree object.
class LitTree : public LitObject {
 public:
  LitTree() = default;
  
  explicit LitTree(const bytes& data);

  // Unlike commit, in-memory tree object stores BYTE form of sha, not
  // hex string. This is done for better memory efficiency
  // (20 bytes instead of one byte per each character of 40char string)
  bytes Serialize() const;

  void Deserialize(const bytes& data);

  // Need to expose internal representation of tree for commands.
  const std::vector<TreeEntry>& GetTreeEntries() const {
    return entries_;
  }

  // Used when building trees from the index (lithub commit).
  // Order doesn't matter, Serialize sorts entries into canonical order.
  void AddEntry(const TreeEntry& entry) {
    entries_.push_back(entry);
  }

  std::string_view Format() const;

 private:
  std::vector<TreeEntry> entries_;
};

#endif  // LIT_TREE_H_
