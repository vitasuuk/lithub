#include "LitRepository.h"
#include "LitConfig.h"

#include <fstream>
#include <stdexcept>

using std::filesystem::exists;
using std::filesystem::is_empty;
using std::filesystem::is_directory;
using std::filesystem::create_directories;
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

LitRepository LitRepository::RepoCreate(const path& path) {
  LitRepository repo = LitRepository(path, true);

  if (exists(repo.worktree_)) {
	  if (!is_directory(path)) {
		  throw std::runtime_error("%s" + path.string() + " is not a directory.");
	  }

	  if (!is_empty(path)) {
		  throw std::runtime_error("%s" + path.string() + " is not empty.");
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
  if (!desc) {
	  throw std::runtime_error("HEAD file failed.");
  }

  head << "ref: refs/heads/master\n";

  repo.config_ = LitConfig::DefaultConfig();
  repo.config_.Write(repo.RepoFile(true, "config"));

  return repo;
}