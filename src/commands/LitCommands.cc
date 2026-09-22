#include "commands/LitCommands.h"
#include "objects/LitBlob.h"
#include "objects/LitCommit.h"
#include "objects/LitObjectStore.h"
#include "objects/LitTag.h"
#include "objects/LitTree.h"
#include "repository/LitRefs.h"
#include "repository/LitRepository.h"
#include "index/LitIndex.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

using std::filesystem::path;

// Private helper methods.
namespace {
  void CmdLogRecurse(LitRepository& repo, 
                     const std::string& commit_sha,
                     std::unordered_set<std::string>& seen);

  void CmdLsTreeRecurse(LitRepository& repo, 
                        const std::string& tree_sha,
                        bool recurse, 
                        const std::string& prefix = "");

  void CmdCheckoutRecurse(LitRepository& repo,
                          LitTree& tree,
                          const std::filesystem::path& dir_path);
  void TagCreate(LitRepository& repo,
                 const std::string& name,
                 const std::string& ref,
                 bool create_tag_object);

  std::string FormatTimestamp(std::uint32_t seconds);
}

int CmdInit(const path& path) {
  LitRepository::RepoCreate(path);
  return 0;
}

int CmdCatFile(const std::string& format, const std::string& object) {
  auto repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  auto sha = ObjectFind(repo, object, format, true);

  if (!sha) {
    throw std::runtime_error(
        "Cannot resolve " + object +
        " to object type " + format);
  }

  std::unique_ptr<LitObject> obj = ObjectRead(repo, sha.value());

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

  auto commit_sha =
      ObjectFind(repo, commit_point, "commit", true);

  if (!commit_sha) {
    throw std::runtime_error(
        "Cannot resolve to a commit: " + commit_point);
  }

  CmdLogRecurse(repo, commit_sha.value(), seen);

  return 0;
}

int CmdLsTree (const std::string& tree_point, bool recurse) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();
  
  auto tree_sha =
      ObjectFind(repo, tree_point, "tree", true);

  if (!tree_sha) {
    throw std::runtime_error(
        "Cannot resolve to a tree: " + tree_point);
  }

  CmdLsTreeRecurse(repo, tree_sha.value(), recurse);

  return 0;
}

int CmdCheckout(const std::string& commit_point, 
                const std::filesystem::path& dir_path) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  auto tree_sha =
      ObjectFind(repo, commit_point, "tree", true);

  if (!tree_sha) {
    throw std::runtime_error(
        "Cannot resolve to a tree: " + commit_point);
  }

  std::unique_ptr<LitObject> obj = ObjectRead(repo, tree_sha.value());

  if (!obj) {
    throw std::runtime_error(
        "Tree object not found: " + tree_sha.value());
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

int CmdTag(bool create_tag_obj,
	         std::optional<std::string> name,
           std::string object) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  if (!name) {
    // List tags
    CmdRefList("refs/tags");
  } else {
    // Create tag name for object or HEAD
    TagCreate(repo, name.value(), object, create_tag_obj);
  }

  return 0;
}

int CmdRevParse(std::string type, std::string name) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  auto sha = ObjectFind(repo, name, type);
  if (!sha) {
    throw std::runtime_error("Cannot resolve " + name + 
      (type.empty() ? "" : " to type" + type));
  }

  std::cout << sha.value() << '\n';
  return 0;
}

int CmdLsFiles(bool verbose) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();
  LitIndex index = IndexRead(repo);

  if (verbose) {
    std::cout << "Index file format v" + index.GetVersion() << "containing "
              << index.GetEntries().size() << " entries.";
  }

  for (const auto& entry : index.GetEntries()) {
    std::cout << entry.name;

    if (verbose) {
      std::string entry_type;

      if (entry.mode_type == 0b1000) {
        entry_type = "regular file";
      } else if (entry.mode_type == 0b1010) {
        entry_type = "symlink";
      } else if (entry.mode_type == 0b1110) {
        entry_type = "git link";
      } else {
        entry_type = "unknown";
      }

      std::cout << "  " << entry_type << " with perms: "
                << std::oct << entry.mode_perms << std::dec << '\n';

      std::cout << "  on blob: " << entry.sha << '\n';

      std::cout << "  created: "
                << FormatTimestamp(entry.ctime.first)
                << "." << entry.ctime.second
                << ", modified: "
                << FormatTimestamp(entry.mtime.first)
                << "." << entry.mtime.second
                << '\n';

      std::cout << "  device: " << entry.dev
                << ", inode: " << entry.ino << '\n';

      std::cout << "  user: " << entry.uid
                << "  group: " << entry.gid << '\n';

      std::cout << "  flags: stage=" << entry.flag_stage
                << " assume_valid=" << entry.flag_assume_valid
                << '\n';
    }
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
      std::cout << "Merge:";

      for (const std::string* parent : parents) {
        std::cout << ' ' << parent->substr(0, 7);
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
        
        std::ofstream file(item_path, std::ios::binary);
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

  void TagCreate(LitRepository& repo,
                 const std::string& name,
                 const std::string& ref,
                 bool create_tag_object) {
    auto sha = ObjectFind(repo, ref, "", true);
    if (!sha) {
      throw std::runtime_error("Reference not found: " + ref);
    }

    std::unique_ptr<LitObject> obj = ObjectRead(repo, sha.value());
    if (!obj) {
      throw std::runtime_error("Object not found: " + sha.value());
    }

    std::string ref_path = "refs/tags/" + name;
    
    if (create_tag_object) {
      LitTag tag = LitTag();

      tag.AddKvlmField("object", sha.value());
      tag.AddKvlmField("type", std::string(obj->Format()));
      tag.AddKvlmField("tag", name);
      // Can let the user use their name once commit is fully implemented
      tag.AddKvlmField("tagger", "lithub <lithub@example.com>");
      // Same with tag message
      tag.SetKvlmMessage("A tag generated by lithub.\n");

      std::string tag_sha = ObjectWrite(tag, &repo);

      RefCreate(repo, ref_path, tag_sha);
    } else {
      // Liteweight tag
      RefCreate(repo, ref_path, sha.value());
    }
  }

  std::string FormatTimestamp(std::uint32_t seconds) {
    std::time_t time = static_cast<std::time_t>(seconds);
    std::tm time_info{};

    #ifdef _WIN32
      localtime_s(&time_info, &time);
    #else
      localtime_r(&time, &time_info);
    #endif

    std::ostringstream oss;
    oss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");

    return oss.str();
  }
}
