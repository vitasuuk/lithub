#include "LitKvlm.h"

#include <algorithm>
#include <stdexcept>

Kvlm KvlmParse(const bytes& raw) {
  Kvlm result;
  std::size_t pos = 0;

  while (pos < raw.size()) {
    if (raw[pos] == '\n') {
      result.message = bytes(raw.begin() + pos + 1, raw.end());
      break;
    }

    auto space_it = std::find(raw.begin() + pos, raw.end(),
        static_cast<std::uint8_t>(' '));
    auto newline_it = std::find(raw.begin() + pos, raw.end(),
        static_cast<std::uint8_t>('\n'));

    if (space_it == raw.end() || newline_it == raw.end() ||
        newline_it < space_it) {
      throw std::runtime_error("Malformed KVLM representation.");
    }

    std::size_t space_pos = static_cast<std::size_t>(space_it - raw.begin());
    std::size_t newline_pos = static_cast<std::size_t>(newline_it - raw.begin());
    
    // Continue through the multi-line values
    while (newline_pos + 1 < raw.size() && raw[newline_pos + 1] == ' ') {
      auto next_nwl = std::find(raw.begin() + newline_pos + 1, raw.end(),
        static_cast<std::uint8_t>('\n'));

      // If we are inside multi-line value and current line doesn't have \n
	    if (next_nwl == raw.end()) {
        throw std::runtime_error("Malformed KVLM representation.");
      }

      newline_pos = static_cast<std::size_t>(next_nwl - raw.begin());
    }

    KvlmField field;

    field.key = bytes(raw.begin() + pos, raw.begin() + space_pos);
    field.value = bytes(raw.begin() + space_pos + 1, raw.begin() + newline_pos);

	  // Remove possible continuation line ' ' markers
    // Note: questionable efficiency
    for (std::size_t i = 0; i < field.value.size() - 1; i++) {
      if (field.value[i] == '\n' && field.value[i + 1] == ' ') {
        field.value.erase(field.value.begin() + i + 1);
      }
    }

    result.fields.push_back(field);

    pos = newline_pos + 1;
  }

  return result;
}

bytes KvlmSerialize(const Kvlm& kvlm) {
  bytes result;
  for (const KvlmField& field : kvlm.fields) {
    result.insert(result.end(), field.key.begin(), field.key.end());

    result.push_back(' ');

    // Inner loop to have add spaces for multi-line value/s
    for (std::uint8_t byte : field.value) {
      result.push_back(byte);

      if (byte == '\n') {
        result.push_back(' ');
      }
    }

    result.push_back('\n');
  }
  
  result.push_back('\n');
  result.insert(result.end(), kvlm.message.begin(), kvlm.message.end());

  return result;
}