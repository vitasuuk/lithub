#ifndef LIT_CONFIG_H_
#define LIT_CONFIG_H_

#include <filesystem>
#include <map>
#include <string>

// INI-style configuration reader/writer used for .lit/config
class LitConfig {
 public:
  LitConfig() = default;

  // Parses INI-style config file into in-memory config representation.
  void Read(const std::filesystem::path& path);

  // Writes the configuration bacj in INI format.
  void Write(const std::filesystem::path& path) const;

  // Returns the value from given section and key.
  // Throws if no such key exists.
  std::string Get(const std::string& section,
                  const std::string& key) const;

  // Creates configuration key-value mapping / replaces value of existing one.
  void Set(const std::string& section,
           const std::string& key,
           const std::string& value);
  
  // Builds and returns config object with default values.
  static LitConfig DefaultConfig();

 private:
  using Section = std::map<std::string, std::string>;

  // Map section name to a mapping of field name -> value
  // Example: [core] <- section name
  //          repositoryformatversion = 0
  //                    ^               ^
  //               field name        field value
  std::map<std::string, Section> cf_sections_;
};

#endif  // LIT_CONFIG_H_
