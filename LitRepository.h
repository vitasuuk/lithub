#ifndef LIT_REPOSITORY_H_
#define LIT_REPOSITORY_H_

#include <filesystem>
#include <string>

//using std::filesystem::path;

class LitRepository {
 public:
  // Constructor
  explicit LitRepository(const std::filesystem::path& path, bool force = false);

  // Create a new repository at path
  static LitRepository RepoCreate(const std::filesystem::path& path);

  // Given the path to the current location, recursively iterate and find
  // the root of repository (location which has .git as direct child)
  static LitRepository RepoFind(const std::filesystem::path& path, bool required = true);
  

 private:
  std::filesystem::path worktree_;
  std::filesystem::path gitdir_;
  
  // Config fields [TBD if separate structure]
  int repo_format_version_ = 0;
  bool file_mode = false;
  bool bare = false;
};


#endif  // LIT_REPOSITORY_H_ 