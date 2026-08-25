#ifndef LIT_OBJECT_H_
#define LIT_OBJECT_H_

#include <cstdint>
#include <string_view>
#include <vector>

using bytes = std::vector<std::uint8_t>;

class LitObject {
 public:
  virtual ~LitObject() = default;

  virtual bytes Serialize() const = 0;

  virtual void Deserialize(const bytes& data) = 0;

  virtual std::string_view Format() const = 0;
};

#endif  // LIT_OBJECT_H_
