#include "LitCommands.h"
#include "LitCommit.h"
#include "LitRepository.h"
#include "LitTree.h"
#include "LitObjectStore.h"

#include <algorithm>

#include <fstream>
#include <iostream>

using std::filesystem::path;

// Private helper methods performing recursion.
namespace {
  void CmdLogRecurse();

  void CmdLsTreeRecurse(LitRepository& repo, const std::string& tree_sha,
                        bool recurse, const std::string& prefix = "");
}

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

int CmdLog (const std::string& commit_point) {

}

// Simplified version without resolving reference.
int CmdLsTree (const std::string& tree_sha, bool recurse) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  CmdLsTreeRecurse(repo, tree_sha, recurse);

  return 0;
}

namespace {
  void CmdLsTreeRecurse(LitRepository& repo, const std::string& tree_sha,
                    bool recurse, const std::string& prefix) {
	  std::unique_ptr<LitObject> obj = ObjectRead(repo, tree_sha);
    LitTree* tree_obj = dynamic_cast<LitTree*>(obj.get());

    if (tree_obj == nullptr) {
      throw std::runtime_error("Object is not a tree: " + tree_sha);
    }

    for (const TreeEntry& entry : tree_obj->GetTreeEntries()) {
      std::string type;

      if (entry.mode == "040000") {
        type = "tree";
      } else if (entry.mode.rfind("10", 0) == 0) {
        type = "blob";
      } else if (entry.mode.rfind("12", 0) == 0) {  // symlink
        type = "blob";
      } else if (entry.mode.rfind("16", 0) == 0) {
        type = "commit";
      } else {
        throw std::runtime_error("Unknown mode: " + entry.mode);
      }

      path full_path = path(prefix) / entry.path;

      if (!recurse || type != "tree") {
        std::cout << entry.mode << " " << type << " " << entry.sha
            << "\t" << full_path.string() << '\n';
      } else {
        CmdLsTreeRecurse(repo, entry.sha, recurse, full_path.string());
      }
    }
  }

  void CmdLogRecurse() {
    
  }
}