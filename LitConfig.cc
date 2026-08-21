#include "LitConfig.h"

#include <fstream>
#include <stdexcept>
#include <string>

using std::filesystem::path;
using std::string;

void LitConfig::Read(const path& path) {
  std::ifstream config_file(path);

  if (!config_file.is_open()) {
    throw std::runtime_error("Couldn't open config file.");
  }

  string current_section;
  string line;

  while (std::getline(config_file, line)) {
    if(line.empty()) {
      continue;
    }

    // Section header
    if (line.front() == '[' && line.back() == ']') {
      current_section = line.substr(1, line.size() - 2);
      continue;
    }

    // Section body
    std::size_t separator = line.find('=');

    if (separator == string::npos) {
      throw std::runtime_error("Invalid configuration entry: "
          + line);
    }

    if (current_section.empty()) {
      throw std::runtime_error("No section name before key-value found.");
    }

    // Works strictly on format "key = value" with spaces in between.
    // Need to do trimming for softer accept.
    string key = line.substr(0, separator - 1);
    string value = line.substr(separator + 2);

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
