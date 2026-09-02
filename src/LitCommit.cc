#include "LitCommit.h"

LitCommit::LitCommit(const bytes& data) {
  Deserialize(data);
}

bytes LitCommit::Serialize() const {
  return KvlmSerialize(kvlm_);
}

void LitCommit::Deserialize(const bytes& data) {
  kvlm_ = KvlmParse(data);
}

std::string_view LitCommit::Format() const {
  return "commit";
}