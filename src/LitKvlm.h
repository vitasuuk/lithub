#ifndef LIT_KVLM_H_
#define LIT_KVLM_H_

#include "LitObject.h"

#include <string>
#include <vector>

/*
  Metadata field for a committ/tag KVLM payload.
  Format:
    parent 010201 -> key = parent, value = 010201
  !!! Field order must be preserved, same keys may appear multiple times !!!
*/
struct KvlmField {
  std::string key;
  std::string value;
};

/*
  Parsed representation of Lit's commit/tag payload format

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
struct Kvlm {
  std::vector<KvlmField> fields;
  std::string message;

  // Returns first value associated with passed key.
  // Throws if no key found.
  const std::string& KvlmGetValue(std::string_view key) const;

  // Returns all values associated with the key.
  // Used, for example, in CmdLog to get multiple parents in case there was
  // a merge.
  // Returns empty vector when no key found.
  std::vector<const std::string*> KvlmGetValues(std::string_view key) const;
};

// Parse raw commit/tag payload bytes into ordered KVLM fields and message.
// Handles multiline values, removes ' ' continuation markers before storing.
Kvlm KvlmParse(const bytes& raw);

// Convert stored KVLM back into the commit/tag payload format.
bytes KvlmSerialize(const Kvlm& kvlm);

#endif  // LIT_KVLM_H_
