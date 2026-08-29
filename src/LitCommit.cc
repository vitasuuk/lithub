#include "LitCommit.h"

LitCommit::LitCommit(const bytes& data) {
  Deserialize(data);
}

bytes LitCommit::Serialize() const {
  return KvlmSerialize(kvlm_);
}

void LitCommit::Deserialize(const bytes& data) {
  KvlmParse(data);
}

std::string_view Format() {
  return "commit";
}