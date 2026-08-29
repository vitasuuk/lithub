#ifndef LIT_KVLM_H_
#define LIT_KVLM_H_

#include "LitObject.h"

#include <string>
#include <vector>

struct KvlmField {
  bytes key;
  bytes value;
};

struct Kvlm {
  std::vector<KvlmField> fields;
  bytes message;
};

Kvlm KvlmParse(const bytes& raw);

bytes KvlmSerialize(const Kvlm& kvlm);

#endif  // LIT_KVLM_H_
