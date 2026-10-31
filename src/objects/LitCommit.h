#ifndef LIT_COMMIT_H_
#define LIT_COMMIT_H_

#include "objects/LitKvlm.h"
#include "objects/LitObject.h"

#include <string>
#include <string_view>
#include <vector>

// Needed for identification purposes and comfortable parsing.
// For example, used in lithub log to display commit info.
struct CommitIdentity {
  std::string author_name;
  std::string email;
  std::string datetime;
  std::string timezone;
};

// Represents a commit object.
// Commit's payload uses the KVLM format:
/*  
  tree 29ff16c9c14e2652b22f8b78bb08a5a07930c147
  parent 206941306e8a8af65b66eaaaea388a7ae24d49a0
  author Thibault Polge <thibault@thb.lt> 1527025023 +0200
  committer Thibault Polge <thibault@thb.lt> 1527025044 +0200
  gpgsig -----BEGIN PGP SIGNATURE-----

   iQIzBAABCAAdFiEExwXquOM8bWb4Q2zVGxM2FxoLkGQFAlsEjZQACgkQGxM2FxoL
   kGQdcBAAqPP+ln4nGDd2gETXjvOpOxLzIMEw4A9gU6CzWzm+oB8mEIKyaH0UFIPh

   -----END PGP SIGNATURE-----

   Create first draft
*/
class LitCommit : public LitObject {
 public:
  LitCommit() = default;

  // Construct commit object by parsing given raw commit payload
  explicit LitCommit(const bytes& data);

  // Convert parsed commit payload into its original byte form
  bytes Serialize() const;

  // Parse raw commit payload into KVLM representation
  void Deserialize(const bytes& data);

  // Return format name
  std::string_view Format() const;

  // Return hex string form of commit's tree sha
  const std::string& GetTreeSha() const;

  // Return vector of hex string form of commit's parents' sha
  std::vector<const std::string*> GetCommitParents() const;

  // Return message
  const std::string& GetCommitMessage() const;

  // Return identity info of commit. Can be used for author and commiter
  // fields.
  CommitIdentity GetIdentityInfo(std::string_view key) const;

  // Builders used to construct a commit/tag from scratch (lithub commit,
  // lithub tag -a). Fields are appended in call order, which is preserved
  // on serialization.
  void AddKvlmField(const std::string& key,
                    const std::string& value) {
    kvlm_.fields.push_back({key, value});
  }

  void SetKvlmMessage(const std::string& message) {
    kvlm_.message = message;
  }

  // Getters for exposure of KVLM needed to set LitTag's KVLM state
  // Only class itself or its children (LitTag) can call it.
 protected:
  Kvlm& GetMutableKvlm() {
    return kvlm_;
  }

  const Kvlm& GetKvlm() const {  // nonmutable for accessing fields from LitTag
    return kvlm_;
  }

 private:
  Kvlm kvlm_;
};

#endif  // LIT_COMMIT_H_
