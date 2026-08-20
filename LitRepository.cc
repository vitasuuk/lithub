#include "LitRepository.h"

#include <stdexcept>

using std::filesystem::exists;
using std::filesystem::is_directory;
using std::filesystem::path;

LitRepository::LitRepository(const path& repo_path,
                             bool force = false)
  : worktree_(repo_path), gitdir_(repo_path / ".git") {
  if (!force && !is_directory(gitdir_)) {
    throw std::runtime_error("Not a Git repository: " + repo_path.string());
  }

  path cf = RepoFile(false, "config");
  
  if (exists(cf)) {
    // to do config reading
  } else if (!force) {
	throw std::runtime_error("Configuration file is missing");
  }

  if (!force && repo_format_version_ != 0) {
	throw std::runtime_error("Unsupported repository format version: %d" +
	                          repo_format_version_);
  }

}