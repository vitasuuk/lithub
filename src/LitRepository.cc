#include "LitRepository.h"
#include "LitConfig.h"

#include <fstream>
#include <stdexcept>

using std::filesystem::canonical;
using std::filesystem::create_directories;
using std::filesystem::exists;
using std::filesystem::is_empty;
using std::filesystem::is_directory;
using std::filesystem::path;

LitRepository::LitRepository(const path& repo_path, bool force)
  : worktree_(repo_path), litdir_(repo_path / ".lit") {
  if (!force && !is_directory(litdir_)) {
    throw std::runtime_error("Not a Lit repository: " + repo_path.string());
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

LitRepository LitRepository::RepoCreate(const path& path) {
  LitRepository repo = LitRepository(path, true);

  if (exists(repo.worktree_)) {
    if (!is_directory(path)) {
      throw std::runtime_error(path.string() + " is not a directory.");
    }

    if (exists(repo.litdir_)) {
      if (!is_directory(repo.litdir_)) {
        throw std::runtime_error(
            repo.litdir_.string() + " is not a directory.");
      }

      if (!is_empty(repo.litdir_)) {
        throw std::runtime_error(repo.litdir_.string() + " is not empty.");
      }
    }
  } else {
    create_directories(repo.worktree_);
  }

  repo.RepoDir(true, "branches");
  repo.RepoDir(true, "objects");
  repo.RepoDir(true, "refs", "tags");
  repo.RepoDir(true, "refs", "heads");

  // .git/description
  std::ofstream desc(repo.RepoFile(true, "description"));
  if (!desc) {
    throw std::runtime_error("Description file failed.");
  }

  desc << "Unnamed repository; edit this file 'description'" 
      " to name the repository.\n";

  //.git/HEAD
  std::ofstream head(repo.RepoFile(true, "HEAD"));
  if (!head) {
    throw std::runtime_error("HEAD file failed.");
  }

  head << "ref: refs/heads/master\n";

  repo.config_ = LitConfig::DefaultConfig();
  repo.config_.Write(repo.RepoFile(true, "config"));

  return repo;
}

std::optional<LitRepository> LitRepository::RepoFind(const path& repo_path, 
                                                     bool required) {
  path resolved = canonical(repo_path);

  // Return condition, found lit root
  if (is_directory(resolved / ".lit")) {
    return LitRepository(resolved);
  }

  // Otherwise
  path parent = canonical(resolved / "..");

  if (parent == resolved) {
    if (required) {
      throw std::runtime_error("No Lit repository found.");
    }

    return std::nullopt;
  }

  return RepoFind(parent, required);
}