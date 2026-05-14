#ifndef LIT_REPOSITORY_H_
#define LIT_REPOSITORY_H_

#include <filesystem>
#include <string>

using std::filesystem::path;

class LitRepository {
 public:
  explicit LitRepository(const std::filesystem::path& path, bool force = false);
 private:


};


#endif  // LIT_REPOSITORY_H_  