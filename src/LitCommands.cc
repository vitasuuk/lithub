#include "LitCommands.h"
#include "LitRepository.h"
#include "LitObjectStore.h"

#include <fstream>
#include <iostream>

using std::filesystem::path;

int CmdInit(const path& path) {
  LitRepository::RepoCreate(path);
  return 0;
}

int CmdCatFile(const std::string& format, const std::string& object) {
  auto repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  std::string sha = ObjectFind(repo, object, format);

  std::unique_ptr<LitObject> obj = ObjectRead(repo, sha);

  if (!obj) {
    throw std::runtime_error("Object not found: " + object);
  }

  bytes data = obj->Serialize();

  std::cout.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
  
  return 0;
}

int CmdHashObject(const std::string& format, const path& file_path,
                  bool write) {
  std::optional<LitRepository> repo_opt;
  LitRepository* repo = nullptr;

  if (write) {
    repo_opt = LitRepository::RepoFind(".");
    repo = &repo_opt.value();
  }

  std::ifstream file(file_path, std::ios::binary);
  
  if (!file) {
    throw std::runtime_error("Failed to open file: " + file_path.string());
  }

  std::string sha = ObjectHash(file, format, repo);

  std::cout << sha << '\n';
  return 0;
}