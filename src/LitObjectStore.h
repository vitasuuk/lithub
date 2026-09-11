/*
  LitObjectStore is responsible for operations common to all object types.

  General object representation operation order:
   1. object payload
   2. <format> <size>\0<payload>
   - ^ SHA1 [for directory path]
   - ^ zlib compress
   3. store at SHA1 hashed path .lit/objects/xx/yyyy...

  Object-specific payload parsing belong to their own classes:
  LitBlob, LitCommit, LitTree.
*/

#ifndef LIT_OBJECT_STORE_H_
#define LIT_OBJECT_STORE_H_

#include "LitObject.h"

#include <string>
#include <memory>
#include <istream>

class LitRepository;

// Serializes an object (<format> <size>\0<payload>), and computes its SHA1.
// If repo == nullptr, only computes and returns SHA,
// otherwise compresses and stores the object in computed by sha path.
std::string ObjectWrite(const LitObject& object,
                        LitRepository* repo = nullptr);

// Reads an object found by given full SHA1 from repo's objects directory
// Performs ObjectWrite in reverse order:
// 1. Read compressed object file
// 2. decompress
// 3. parse object header
// 4. Construct and return appropriate LitObject subclass.
std::unique_ptr<LitObject> ObjectRead(LitRepository& repo,
                                      const std::string& sha);

// Resolves user's object name into an object SHA1.
// [Placeholder currently, to be implemented]
std::optional<std::string> ObjectFind(LitRepository& repo, 
                                      const std::string& name,
                                      const std::string& format, 
                                      bool follow = true);

// Reads raw bytes from input, constructs requested object type and passes to
// ObjectWrite()
std::string ObjectHash(std::istream& input,
                       const std::string& format,
                       LitRepository* repo = nullptr);
		  
#endif  // LIT_OBJECT_STORE_H_
