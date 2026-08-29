#ifndef LIT_BLOB_H_
#define LIT_BLOB_H_

#include "LitObject.h"

class LitBlob : public LitObject {
 public:
  LitBlob() = default;

  explicit LitBlob(const bytes& data);

  bytes Serialize() const;

  void Deserialize(const bytes& data);

  std::string_view Format() const;

 private:
  bytes data_;
};

#endif
