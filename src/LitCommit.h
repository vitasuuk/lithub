#ifndef GIT_COMMIT_H_
#define GIT_COMMIT_H_

#include "LitObject.h"
#include "LitKvlm.h"

class LitCommit : LitObject {
 public:
  LitCommit() = default;

  explicit LitCommit(const bytes& data);

  bytes Serialize() const;

  void Deserialize(const bytes& data);

  std::string_view Format() const;
 
 private:
  Kvlm kvlm_;
};

#endif  // GIT_COMMIT_H_
