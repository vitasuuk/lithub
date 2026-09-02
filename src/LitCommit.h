#ifndef LIT_COMMIT_H_
#define LIT_COMMIT_H_

#include "LitObject.h"
#include "LitKvlm.h"

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

  // Getter needed for accessing commit object's parsed data from
  // LitObjectStore.cc
  const Kvlm& GetKvlm() const {
    return kvlm_;
  }
 
 private:
  Kvlm kvlm_;
};

#endif  // LIT_COMMIT_H_
