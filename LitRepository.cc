#include "LitRepository.h"
#include "LitConfig.h"

#include <fstream>
#include <stdexcept>

using std::filesystem::exists;
using std::filesystem::is_directory;
using std::filesystem::path;

LitRepository::LitRepository(const path& repo_path, bool force)
  : worktree_(repo_path), gitdir_(repo_path / ".git") {
  if (!force && !is_directory(gitdir_)) {
    throw std::runtime_error("Not a Git repository: " + repo_path.string());
  }

  path config_path = RepoFile(false, "config");
  
  if (exists(config_path)) {
    config_.Read(config_path);
  } else if (!force) {
	throw std::runtime_error("Configuration file is missing.");
  }

  if (!force) {
	int version = std::stoi(config_.Get("core", "repositoryformatversion"));

	if (version != 0) {
		throw std::runtime_error("Unsupported repositoryformatversion: " +
		    std::to_string(version));
	}
  }
}