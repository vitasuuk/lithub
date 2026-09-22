#ifndef LIT_REPOSITORY_H_
#define LIT_REPOSITORY_H_

#include "repository/LitConfig.h"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

/*
  Represents one Lit repository in file system.
  This class is responsible solely for repository layout and path operations.
*/
class LitRepository {
 public:
  // Open/represent an existing Lit repository. 
  // When force == true, lit requires .lit to exist, reads .lit/config,
  // validates format version
  //      force == false is used internally by RepoCreate before .lit exists.
  explicit LitRepository(const std::filesystem::path& path,
                         bool force = false);

  // Initializes a new Lit repository at path.
  // Creates the .lit directory structure and default metadata files, then
  // returns a LitRepository object representing it.
  static LitRepository RepoCreate(const std::filesystem::path& path);

  // Starting from path, recursively search upward for a repository
  // (parent directory of .lit)
  // When required == true, throws if not found
  // Otherwise, returns std::nullopt.
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
