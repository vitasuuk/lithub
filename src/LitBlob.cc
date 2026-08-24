#include "LitBlob.h"

LitBlob::LitBlob(const bytes& data) {
  Deserialize(data);
}

bytes LitBlob::Serialize() const {
  return data_;
}

void LitBlob::Deserialize(const bytes& data) {
  data_ = data;
}

std::string_view LitBlob::Format() const {
  return "blob";
}
