#ifndef LIT_BLOB_H_
#define LIT_BLOB_H_

#include "LitObject.h"

// Represents a blob object, containing only raw contents of file.
//
// A blob does not know the original filename or directory. Stores only the
// file's contents.
class LitBlob : public LitObject {
 public:
  LitBlob() = default;

  // Construct blob object directly from raw contents
  explicit LitBlob(const bytes& data);

  // Blob's payload is simply its raw content, so serialize returns
  // its data unchanged.
  bytes Serialize() const;

  // Store blob's payload in the object.
  void Deserialize(const bytes& data);

  // Return format name
  std::string_view Format() const;

 private:
  bytes data_;
};

#endif
