#include "LitCommit.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
  // Converts timezone format into seconds offset needed for epoch time
  // convertion and adjustment to user's local time.
  int ParseTimeZoneOffset(const std::string& timezone);

  // Converts epoch timestamp to date time format string .
  std::string FormatDateTime(const std::string& timestamp,
                             const std::string& timezone);
}

LitCommit::LitCommit(const bytes& data) {
  Deserialize(data);
}

bytes LitCommit::Serialize() const {
  return KvlmSerialize(kvlm_);
}

void LitCommit::Deserialize(const bytes& data) {
  kvlm_ = KvlmParse(data);
}

std::string_view LitCommit::Format() const {
  return "commit";
}

const std::string& LitCommit::GetTreeSha() const {
  return kvlm_.KvlmGetValue("tree");
}

std::vector<const std::string*> LitCommit::GetCommitParents() const {
  return kvlm_.KvlmGetValues("parent");
}

const std::string& LitCommit::GetCommitMessage() const {
  return kvlm_.message;
}

CommitIdentity LitCommit::GetIdentityInfo(std::string_view key) const {
  CommitIdentity identity;
  const std::string& identity_field = kvlm_.KvlmGetValue(key);

  std::size_t email_start = identity_field.find('<');
  std::size_t email_end = identity_field.find('>', email_start);

  identity.author_name = identity_field.substr(0, email_start - 1);
  identity.email = identity_field.substr(email_start + 1,
      email_end - email_start - 1);  // skip <>, store email itself

  std::size_t timestamp_start = email_end + 2;
  std::size_t timestamp_end = identity_field.find(' ', timestamp_start);

  std::string timestamp = identity_field.substr(timestamp_start,
      timestamp_end - timestamp_start);

  identity.timezone = identity_field.substr(timestamp_end + 1);
  identity.datetime = FormatDateTime(timestamp, identity.timezone);

  return identity;
}

namespace {

  int ParseTimeZoneOffset(const std::string& timezone) {
    if (timezone.size() != 5 || (timezone[0] != '+' && timezone[0] != '-')) {
      throw std::runtime_error("Invalid timezone format: " + timezone);
    }

    int hours = std::stoi(timezone.substr(1,2));
    int minutes = std::stoi(timezone.substr(3,2));

    int offset_seconds = hours * 3600 + minutes * 60;

    if (timezone[0] == '-') {
      offset_seconds *= -1;
    }

    return offset_seconds;
  }

  std::string FormatDateTime(const std::string& timestamp_str, 
                                   const std::string& timezone) {
    std::time_t timestamp = static_cast<std::time_t>(
        std::stoll(timestamp_str));

    int offset = ParseTimeZoneOffset(timezone);
    std::time_t display_time = timestamp + offset;

    std::tm time_info{};

    #if defined(_WIN32) || defined(_WIN64)
      if (gmtime_s(&time_info, &display_time) != 0) {
        throw std::runtime_error("Failed to convert timestamp.");
      }
    #else
      if (gmtime_r(&display_time, &time_info) == nullptr) {
        throw std::runtime_error("Failed to convert timestamp.");
      }
    #endif

    std::ostringstream oss;

    oss << std::put_time(&time_info, "%a %b %e %H:%M:%S %Y");

    return oss.str();
  }

}