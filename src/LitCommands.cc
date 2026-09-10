#include "LitBlob.h"
#include "LitCommands.h"
#include "LitCommit.h"
#include "LitRefs.h"
#include "LitRepository.h"
#include "LitTree.h"
#include "LitObjectStore.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>
#include <iostream>
#include <map>

using std::filesystem::path;

// Private helper methods.
namespace {
  void CmdLogRecurse(LitRepository& repo, const std::string& commit_sha,
                     std::unordered_set<std::string>& seen);

  void CmdLsTreeRecurse(LitRepository& repo, const std::string& tree_sha,
                        bool recurse, const std::string& prefix = "");

  void CmdCheckoutRecurse(LitRepository& repo,
                          LitTree& tree,
                          const std::filesystem::path& dir_path);
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
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();
  std::unordered_set<std::string> seen;

  CmdLogRecurse(repo, commit_point, seen);

  return 0;
}

int CmdLsTree (const std::string& tree_sha, bool recurse) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();
  

  CmdLsTreeRecurse(repo, tree_sha, recurse);

  return 0;
}

int CmdCheckout(const std::string& commit_sha, 
                const std::filesystem::path& dir_path) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  std::unique_ptr<LitObject> obj = ObjectRead(repo, commit_sha);
  
  // if the object is commit, we grab its tree
  if (obj->Format() == "commit") {
    LitCommit* commit_obj = dynamic_cast<LitCommit*>(obj.get());
    obj = ObjectRead(repo, commit_obj->GetTreeSha());
  }

  LitTree* tree_obj = dynamic_cast<LitTree*>(obj.get());
  if (tree_obj == nullptr) {
    throw std::runtime_error("Cannot resolve to a tree.");
  }

  // Verify that given path is an empty directory / create such
  if (std::filesystem::exists(dir_path)) {
    if (!std::filesystem::is_directory(dir_path)) {
      throw std::runtime_error("Not a directory: " + dir_path.string());
    }
    if (!std::filesystem::is_empty(dir_path)) {
      throw std::runtime_error("Not an empty directory: " + dir_path.string());
    }
  } else {
    std::filesystem::create_directories(dir_path);
  }
 
  // Recurse to reconstruct full worktree
  CmdCheckoutRecurse(repo, *tree_obj, dir_path);

  return 0;
}

int CmdRefList(const std::string& ref_dir) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  std::map<std::string, std::string> ref_list = RefList(repo, ref_dir);

  for (auto& pair : ref_list) {
    std::cout << pair.second << ' ' << pair.first << '\n';
  }

  return 0;
}

namespace {

  void CmdLogRecurse(LitRepository& repo, const std::string& commit_sha,
                     std::unordered_set<std::string>& seen) {
    // Skip commits already visited through another merge branch.
    if (seen.count(commit_sha) != 0) {
      return;
    }

    seen.insert(commit_sha);

    std::unique_ptr<LitObject> obj = ObjectRead(repo, commit_sha);
    LitCommit* commit_obj = dynamic_cast<LitCommit*>(obj.get());
    
    if (commit_obj == nullptr) {
      throw std::runtime_error("Object is not a commit:" + commit_sha);
    }

    std::vector<const std::string*> parents = commit_obj->GetCommitParents();

    std::cout << "commit " << commit_sha << '\n';
    if (parents.size() > 1) {
      std::cout << "Merge: ";

      for (const std::string* parent : parents) {
        std::cout << parent->substr(0, 7);
      }

      std::cout << '\n';
    }

    CommitIdentity author = commit_obj->GetIdentityInfo("author");
    std::cout << "Author: " << author.author_name << " <"
              << author.email << ">\n";
    std::cout << "Date:   " << author.datetime << ' '
              << author.timezone << '\n';
    // @TODO tab before every line of commit message.
    std::cout << "\n\t" << commit_obj->GetCommitMessage() << "\n\n";
    
    // Recurse up towards the initial commit. Handles merge logic.
    // Initial commit has no parent field, GetCommitParents returns empty
    // vector in that case - loop iterates 0 times.
    for (const std::string* parent : parents) {
      CmdLogRecurse(repo, *parent, seen);
    }
  }

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

  void CmdCheckoutRecurse(LitRepository& repo,
                          LitTree& tree,
                          const std::filesystem::path& dir_path) {
    for (const TreeEntry& entry : tree.GetTreeEntries()) {
      std::unique_ptr<LitObject> obj = ObjectRead(repo, entry.sha);
      path item_path = dir_path / entry.path;

      if (obj->Format() == "tree") {
        std::filesystem::create_directory(item_path);
        LitTree* tree_obj = dynamic_cast<LitTree*>(obj.get());

        CmdCheckoutRecurse(repo, *tree_obj, item_path);
      } else if (obj->Format() == "blob") {
        LitBlob* blob_obj = dynamic_cast<LitBlob*>(obj.get());
        bytes data = blob_obj->Serialize();
        
        std::ofstream file(item_path);
        if (!file) {
          throw std::runtime_error("Failed to create a file: " +
              item_path.string());
        }

        file.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));

        if (!file) {
          throw std::runtime_error("Failed to write file: " +
              item_path.string());
        }
      }
    }
  }

}