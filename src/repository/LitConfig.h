#ifndef LIT_CONFIG_H_
#define LIT_CONFIG_H_

#include <filesystem>
#include <map>
#include <optional>
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
  
  // Returns true if given section contains the key.
  bool Has(const std::string& section,
           const std::string& key) const;

  // Returns "Name <email>" built from [user] name and email,
  // or nullopt if either is missing.
  std::optional<std::string> UserIdentity() const;

  // Builds and returns config object with default values.
  static LitConfig DefaultConfig();

  // Reads the user's global config, same files git uses:
  //   $XDG_CONFIG_HOME/git/config (or ~/.config/git/config)
  //   ~/.gitconfig
  // Missing files are skipped.
  static LitConfig GlobalConfig();

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
