#ifndef LIT_OBJECT_H_
#define LIT_OBJECT_H_

#include <cstdint>
#include <string_view>
#include <vector>

// Raw binary data used for objects' contents.
// Using vector rather than string for aribtrary binary data
using bytes = std::vector<std::uint8_t>;

// Abstract base class for every type of object stored in .lit/objects
//
// LitObject handles the object-specific payload only
class LitObject {
 public:
  virtual ~LitObject() = default;

  // Convert object's representation into its raw payload
  virtual bytes Serialize() const = 0;

  // Set this object's representation from a raw payload
  virtual void Deserialize(const bytes& data) = 0;

  // Returns object type name for constructing object header
  virtual std::string_view Format() const = 0;
};

#endif  // LIT_OBJECT_H_
