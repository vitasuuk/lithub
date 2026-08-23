#ifndef LIT_CONFIG_H_
#define LIT_CONFIG_H_

#include <filesystem>
#include <map>
#include <string>

class LitConfig {
 public:
  LitConfig() = default;

  void Read(const std::filesystem::path& path);

  void Write(const std::filesystem::path& path) const;

  std::string Get(const std::string& section,
                  const std::string& key) const;

  void Set(const std::string& section,
           const std::string& key,
           const std::string& value);

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
