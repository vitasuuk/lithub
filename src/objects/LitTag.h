#ifndef LIT_TAG_H_
#define LIT_TAG_H_

#include "objects/LitCommit.h"

#include <string>
#include <string_view>

// LitTag is practically indentical to LitCommit object.
// @TODO figure out the way to construct it since we won't have
// raw bytes when we want to create 
class LitTag : public LitCommit {
 public:
  using LitCommit::LitCommit;

  std::string_view Format() const {
	return "tag";
  }

  const std::string& GetObjectSha() const {
    return GetKvlm().KvlmGetValue("object");
  }
};

#endif  // LIT_TAG_H_
