#include "repository/LitConfig.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using std::filesystem::path;
using std::string;

namespace {

  // Strips leading and trailing whitespace (spaces, tabs, \r).
  string Trim(const string& str) {
    const char* whitespace = " \t\r\n";
    std::size_t start = str.find_first_not_of(whitespace);

    if (start == string::npos) {
      return "";
    }

    std::size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
  }

}

void LitConfig::Read(const path& path) {
  std::ifstream config_file(path);

  if (!config_file.is_open()) {
    throw std::runtime_error("Couldn't open config file.");
  }

  string current_section;
  string line;

  while (std::getline(config_file, line)) {
    // Git's own config files (~/.gitconfig) indent keys with tabs,
    // so trim before parsing anything.
    line = Trim(line);

    // Skip empty lines and comments
    if (line.empty() || line.front() == '#' || line.front() == ';') {
      continue;
    }

    // Section header
    if (line.front() == '[' && line.back() == ']') {
      current_section = line.substr(1, line.size() - 2);
      continue;
    }

    if (current_section.empty()) {
      throw std::runtime_error("No section name before key-value found.");
    }

    // Section body
    std::size_t separator = line.find('=');

    // Git allows a bare key, which means boolean true.
    if (separator == string::npos) {
      cf_sections_[current_section][line] = "true";
      continue;
    }

    string key = Trim(line.substr(0, separator));
    string value = Trim(line.substr(separator + 1));

    // name = "John Doe" -> John Doe
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
      value = value.substr(1, value.size() - 2);
    }

    cf_sections_[current_section][key] = value;
  }
}

void LitConfig::Write(const path& path) const {
  std::ofstream config_file(path);

  if (!config_file) {
    throw std::runtime_error("Failed to write in configuration file: " +
        path.string());
  }

  for (const auto& it : cf_sections_) {
    config_file << "[" << it.first << "]\n";

    for (const auto& subit : it.second) {
      config_file << subit.first << " = " << subit.second << "\n";
    }

    config_file << "\n";
  }
}

void LitConfig::Set(const string& section,
                    const string& key,
                    const string& value) {
  cf_sections_[section][key] = value;
}

// Returns value of field mapped to the section.
// Note: using .at() to prevent operator[] from creating
//       non-existing map entries.
string LitConfig::Get(const string& section,
                    const string& key) const {
  return cf_sections_.at(section).at(key);
}

bool LitConfig::Has(const string& section, const string& key) const {
  auto section_it = cf_sections_.find(section);

  return section_it != cf_sections_.end() &&
         section_it->second.count(key) != 0;
}

std::optional<string> LitConfig::UserIdentity() const {
  if (Has("user", "name") && Has("user", "email")) {
    return Get("user", "name") + " <" + Get("user", "email") + ">";
  }

  return std::nullopt;
}

LitConfig LitConfig::GlobalConfig() {
  LitConfig config;

  const char* home = std::getenv("HOME");
  const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");

  std::vector<path> config_files;

  if (xdg_config_home != nullptr && *xdg_config_home != '\0') {
    config_files.push_back(path(xdg_config_home) / "git" / "config");
  } else if (home != nullptr) {
    config_files.push_back(path(home) / ".config" / "git" / "config");
  }

  if (home != nullptr) {
    config_files.push_back(path(home) / ".gitconfig");
  }

  // Later files override earlier ones, since Read() doesn't clear
  // already parsed values.
  for (const path& file : config_files) {
    if (std::filesystem::is_regular_file(file)) {
      config.Read(file);
    }
  }

  return config;
}

LitConfig LitConfig::DefaultConfig() {
  LitConfig config;

  config.Set("core", "repositoryformatversion", "0");
  config.Set("core", "filemode", "false");
  config.Set("core", "bare", "false");

  return config;
}
