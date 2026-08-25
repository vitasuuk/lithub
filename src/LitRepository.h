#ifndef LIT_REPOSITORY_H_
#define LIT_REPOSITORY_H_

#include "LitConfig.h"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

class LitRepository {
 public:
  // Constructor 
  // (explicit to ensure object isn't initialized implicitly.
  // Ex.: LitRepository = "path";)
  explicit LitRepository(const std::filesystem::path& path,
                         bool force = false);

  // Create a new repository at path
  static LitRepository RepoCreate(const std::filesystem::path& path);

  // Starting from path, recursively search upward for a repository
  // (location which has .lit as direct child).
  // Required default = true: .lit must exist at given path
  static std::optional<LitRepository> RepoFind(const std::filesystem::path& path,
                                               bool required = true);
  
  /*
    Implementations of variadic methods in .h file for other
    .cc files to be able to use them. Refer to visibility of
    template methods between .cc files.
  */
  
  // Returns a path relative to .lit directory
  // Example:
  //  repo.RepoPath("something", "from", "project")
  // Result: <worktreee>/.lit/something/from/project
  template <typename... Paths> // variadic template
  std::filesystem::path RepoPath(Paths&&... path) {
    std::filesystem::path result = litdir_;
    (result /= ... /= path); // folded statement

    return result;
  }
  // Returns a path to a file, optionally creates directories.
  // Note: creates directories up to the last component.
  template <typename... Paths>
  std::filesystem::path RepoFile(bool mkdir = false,
                                  Paths&&... path) {
    std::filesystem::path full_path = RepoPath(path...);
    std::filesystem::path parent = full_path.parent_path();

    if (std::filesystem::exists(parent)) {
      if (std::filesystem::is_directory(parent)) {
        return full_path;
      } else {
        throw std::runtime_error("Not a directory: " + parent.string());
      }
    }

    if (mkdir) {
      std::filesystem::create_directories(parent);
      return full_path;
    }

    return {};
  }

  // Returns a path to an existing directory inside the repository's 
  // .lit directory. If mkdir is true creates directories and returns the path.
  template <typename... Paths>
  std::filesystem::path RepoDir(bool mkdir = false,
                                 Paths&&... path) {
    std::filesystem::path full_path = RepoPath(path...);

    // If such path already exists in filesystem, check
    // if it is directory
    if (std::filesystem::exists(full_path)) {
      if (std::filesystem::is_directory(full_path)) {
        return full_path;
      } else {
          throw std::runtime_error("Not a directory: " + full_path.string());
      }
    }
      
    // Since it doesn't exist, optionally create directories
    if (mkdir) {
      std::filesystem::create_directories(full_path);
      return full_path;
    }

    return {};
  }

 private:
  std::filesystem::path worktree_;
  std::filesystem::path litdir_;
  LitConfig config_;
};

#endif  // LIT_REPOSITORY_H_
