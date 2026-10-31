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
#include <ctime>
#include <fstream>
#include <unordered_set>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
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

  // Status helpers
  void CmdStatusBranch(LitRepository& repo);

  void CmdStatusHeadIndex(LitRepository& repo, const LitIndex& index);

  void CmdStatusIndexWorktree(LitRepository& repo, const LitIndex& index);

  // Recursively flattens a tree into a mapping of
  // full path ("src/main.cc") -> blob sha.
  void TreeToMap(LitRepository& repo,
                 const std::string& tree_sha,
                 std::map<std::string, std::string>& result,
                 const std::string& prefix = "");

  // Converts a user given path into a path relative to the worktree root
  // in index format ("src/main.cc"). Throws if path is outside the worktree
  // or inside .lit
  std::string WorktreeRelative(const LitRepository& repo,
                               const std::string& user_path);

  // Removes paths from the index. With delete_files also removes them from
  // the worktree. With skip_missing, paths that aren't in the index are
  // silently ignored instead of throwing.
  void Rm(LitRepository& repo,
          const std::vector<std::string>& paths,
          bool delete_files = true,
          bool skip_missing = false);

  void Add(LitRepository& repo, const std::vector<std::string>& paths);

  // Builds tree objects (one per directory) from the flat index,
  // writes them to the object store and returns the root tree's sha.
  std::string TreeFromIndex(LitRepository& repo, const LitIndex& index);

  // Creates and writes a commit object, returns its sha.
  std::string CommitCreate(LitRepository& repo,
                           const std::string& tree_sha,
                           const std::optional<std::string>& parent,
                           const std::string& author,
                           std::time_t timestamp,
                           const std::string& message);

  // Formats local timezone offset at given time as git does: +0200, -0700
  std::string FormatTimezone(std::time_t timestamp);

  std::string Trim(const std::string& str);
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
    std::cout << "Index file format v" << index.GetVersion() << ", containing "
              << index.GetEntries().size() << " entries.\n";
  }

  for (const auto& entry : index.GetEntries()) {
    std::cout << entry.name << '\n';

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

int CmdStatus() {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();
  LitIndex index = IndexRead(repo);

  CmdStatusBranch(repo);
  CmdStatusHeadIndex(repo, index);
  std::cout << '\n';
  CmdStatusIndexWorktree(repo, index);

  return 0;
}

int CmdRm(const std::vector<std::string>& paths) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  Rm(repo, paths);

  return 0;
}

int CmdAdd(const std::vector<std::string>& paths) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();

  Add(repo, paths);

  return 0;
}

int CmdCommit(const std::string& message) {
  std::optional<LitRepository> repo_opt = LitRepository::RepoFind(".");
  LitRepository& repo = repo_opt.value();
  LitIndex index = IndexRead(repo);

  std::optional<std::string> author = LitConfig::GlobalConfig().UserIdentity();
  if (!author) {
    throw std::runtime_error(
        "Author identity unknown. Set user.name and user.email in "
        "~/.gitconfig");
  }

  // Create trees, grab back SHA for the root tree.
  std::string tree_sha = TreeFromIndex(repo, index);

  // HEAD doesn't resolve on a fresh repository: first commit has no parent.
  std::optional<std::string> parent = RefResolve(repo, "HEAD");

  std::string commit_sha = CommitCreate(repo, tree_sha, parent,
                                        author.value(), std::time(nullptr),
                                        message);

  // Update HEAD so our commit is now the tip of the active branch.
  std::optional<std::string> active_branch = BranchGetActive(repo);

  if (active_branch) {
    std::string ref_path = "refs/heads/" + active_branch.value();
    RefCreate(repo, ref_path, commit_sha);
  } else {
    // Detached HEAD, move HEAD itself.
    std::ofstream head(repo.RepoFile(false, "HEAD"));
    if (!head) {
      throw std::runtime_error("Could not write HEAD.");
    }

    head << commit_sha << '\n';
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

  void CmdStatusBranch(LitRepository& repo) {
    std::optional<std::string> branch = BranchGetActive(repo);

    if (branch) {
      std::cout << "On branch " << branch.value() << ".\n";
    } else {
      std::cout << "HEAD detached at "
                << ObjectFind(repo, "HEAD", "").value() << '\n';
    }
  }

  void CmdStatusHeadIndex(LitRepository& repo, const LitIndex& index) {
    std::cout << "Changes to be committed:\n";

    // On a fresh repository HEAD points to a branch that doesn't
    // exist yet, so everything in the index counts as added.
    std::map<std::string, std::string> head;
    std::optional<std::string> head_sha = RefResolve(repo, "HEAD");

    if (head_sha) {
      std::optional<std::string> tree_sha =
          ObjectFind(repo, head_sha.value(), "tree", true);

      TreeToMap(repo, tree_sha.value(), head);
    }

    for (const LitIndexEntry& entry : index.GetEntries()) {
      auto it = head.find(entry.name);

      if (it != head.end()) {
        if (it->second != entry.sha) {
          std::cout << "  modified: " << entry.name << '\n';
        }

        head.erase(it);
      } else {
        std::cout << "  added:    " << entry.name << '\n';
      }
    }

    // Paths still left in HEAD weren't met in the index, so they
    // have been deleted.
    for (const auto& pair : head) {
      std::cout << "  deleted:  " << pair.first << '\n';
    }
  }

  void CmdStatusIndexWorktree(LitRepository& repo, const LitIndex& index) {
    std::cout << "Changes not staged for commit:\n";

    const path& worktree = repo.Worktree();

    // Walk the worktree (skipping .lit) and collect every file's path
    // relative to the root. std::set keeps output sorted.
    std::set<std::string> all_files;

    for (auto it = std::filesystem::recursive_directory_iterator(worktree);
         it != std::filesystem::recursive_directory_iterator(); ++it) {
      if (it->path() == repo.Litdir()) {
        it.disable_recursion_pending();
        continue;
      }

      if (!it->is_directory()) {
        all_files.insert(it->path().lexically_relative(worktree)
                                   .generic_string());
      }
    }

    // Compare real files with their cached versions in the index.
    for (const LitIndexEntry& entry : index.GetEntries()) {
      path full_path = worktree / entry.name;

      if (!std::filesystem::exists(full_path)) {
        std::cout << "  deleted:  " << entry.name << '\n';
      } else {
        LitIndexEntry current = IndexEntryFromFile(full_path, entry.name, "");

        // Cheap check first: if metadata is unchanged, assume content is
        // too. Otherwise, rehash the file and compare shas.
        if (current.ctime != entry.ctime || current.mtime != entry.mtime) {
          std::ifstream file(full_path, std::ios::binary);
          if (!file) {
            throw std::runtime_error("Failed to open file: " +
                full_path.string());
          }

          std::string new_sha = ObjectHash(file, "blob", nullptr);

          if (new_sha != entry.sha) {
            std::cout << "  modified: " << entry.name << '\n';
          }
        }
      }

      all_files.erase(entry.name);
    }

    // Whatever is left is not tracked.
    // @TODO filter through ignore rules once LitIgnore is done.
    std::cout << "\nUntracked files:\n";

    for (const std::string& file : all_files) {
      std::cout << "  " << file << '\n';
    }
  }

  void TreeToMap(LitRepository& repo,
                 const std::string& tree_sha,
                 std::map<std::string, std::string>& result,
                 const std::string& prefix) {
    std::unique_ptr<LitObject> obj = ObjectRead(repo, tree_sha);
    LitTree* tree_obj = dynamic_cast<LitTree*>(obj.get());

    if (tree_obj == nullptr) {
      throw std::runtime_error("Object is not a tree: " + tree_sha);
    }

    for (const TreeEntry& entry : tree_obj->GetTreeEntries()) {
      std::string full_path = (path(prefix) / entry.path).generic_string();

      if (entry.mode == "040000") {
        TreeToMap(repo, entry.sha, result, full_path);
      } else {
        result[full_path] = entry.sha;
      }
    }
  }

  std::string WorktreeRelative(const LitRepository& repo,
                               const std::string& user_path) {
    // absolute() first: weakly_canonical keeps a relative path relative if
    // none of its components exist (e.g. rm of an already deleted file).
    path abs_path = std::filesystem::weakly_canonical(
        std::filesystem::absolute(user_path));
    path rel_path = abs_path.lexically_relative(repo.Worktree());

    if (rel_path.empty() || rel_path == "." || *rel_path.begin() == "..") {
      throw std::runtime_error("Path is outside of the worktree: " +
          user_path);
    }

    if (*rel_path.begin() == ".lit") {
      throw std::runtime_error("Path is inside .lit: " + user_path);
    }

    return rel_path.generic_string();
  }

  void Rm(LitRepository& repo,
          const std::vector<std::string>& paths,
          bool delete_files,
          bool skip_missing) {
    LitIndex index = IndexRead(repo);

    std::set<std::string> rel_paths;
    for (const std::string& user_path : paths) {
      rel_paths.insert(WorktreeRelative(repo, user_path));
    }

    std::vector<LitIndexEntry> kept_entries;
    std::vector<path> remove;

    for (const LitIndexEntry& entry : index.GetEntries()) {
      auto it = rel_paths.find(entry.name);

      if (it != rel_paths.end()) {
        remove.push_back(repo.Worktree() / entry.name);
        rel_paths.erase(it);
      } else {
        kept_entries.push_back(entry);
      }
    }

    // Anything left wasn't found in the index.
    if (!rel_paths.empty() && !skip_missing) {
      std::string message = "Cannot remove paths not in the index:";

      for (const std::string& rel_path : rel_paths) {
        message += " " + rel_path;
      }

      throw std::runtime_error(message);
    }

    if (delete_files) {
      for (const path& file : remove) {
        std::filesystem::remove(file);
      }
    }

    index.GetMutableEntries() = kept_entries;
    IndexWrite(repo, index);
  }

  void Add(LitRepository& repo, const std::vector<std::string>& paths) {
    // Validate everything before touching the index, so a bad path doesn't
    // leave it half-modified.
    std::set<std::string> rel_paths;

    for (const std::string& user_path : paths) {
      if (!std::filesystem::is_regular_file(user_path)) {
        throw std::runtime_error("Not a file: " + user_path);
      }

      rel_paths.insert(WorktreeRelative(repo, user_path));
    }

    // Drop existing entries for these paths, keep files on disk.
    Rm(repo, paths, false, true);

    // Re-read the index modified by Rm.
    LitIndex index = IndexRead(repo);
    std::vector<LitIndexEntry>& entries = index.GetMutableEntries();

    for (const std::string& rel_path : rel_paths) {
      path full_path = repo.Worktree() / rel_path;

      std::ifstream file(full_path, std::ios::binary);
      if (!file) {
        throw std::runtime_error("Failed to open file: " + full_path.string());
      }

      std::string sha = ObjectHash(file, "blob", &repo);

      entries.push_back(IndexEntryFromFile(full_path, rel_path, sha));
    }

    // Git keeps index entries sorted by path.
    std::sort(entries.begin(), entries.end(),
              [](const LitIndexEntry& a, const LitIndexEntry& b) {
                return a.name < b.name;
              });

    IndexWrite(repo, index);
  }

  std::string TreeFromIndex(LitRepository& repo, const LitIndex& index) {
    // Directory relative to the worktree ("" is root) -> its tree's entries.
    std::map<std::string, std::vector<TreeEntry>> contents;
    contents[""];

    for (const LitIndexEntry& entry : index.GetEntries()) {
      path entry_path(entry.name);
      std::string dirname = entry_path.parent_path().generic_string();

      // Create map entries for every directory up to root, since even a
      // directory with no files directly in it still holds a tree.
      for (path key = entry_path.parent_path(); !key.empty();
           key = key.parent_path()) {
        contents[key.generic_string()];
      }

      // Index stores mode as integers, tree needs octal ASCII:
      // type 0b1000, perms 0644 -> "100644"
      std::ostringstream mode;
      mode << std::oct << std::setfill('0')
           << std::setw(2) << entry.mode_type
           << std::setw(4) << entry.mode_perms;

      contents[dirname].push_back(
          {mode.str(), entry_path.filename().string(), entry.sha});
    }

    // Sort directories by length, descending. This way every directory
    // is processed before its parent, so its tree sha is known by the time
    // the parent's tree is written. Root ("") always ends up last.
    std::vector<std::string> sorted_paths;
    for (const auto& pair : contents) {
      sorted_paths.push_back(pair.first);
    }

    std::sort(sorted_paths.begin(), sorted_paths.end(),
              [](const std::string& a, const std::string& b) {
                return a.size() > b.size();
              });

    std::string sha;

    for (const std::string& dir : sorted_paths) {
      LitTree tree;

      for (const TreeEntry& entry : contents[dir]) {
        tree.AddEntry(entry);
      }

      sha = ObjectWrite(tree, &repo);

      if (dir.empty()) {
        break;  // root
      }

      // Register this tree in its parent directory.
      path dir_path(dir);
      contents[dir_path.parent_path().generic_string()].push_back(
          {"040000", dir_path.filename().string(), sha});
    }

    return sha;
  }

  std::string CommitCreate(LitRepository& repo,
                           const std::string& tree_sha,
                           const std::optional<std::string>& parent,
                           const std::string& author,
                           std::time_t timestamp,
                           const std::string& message) {
    LitCommit commit;

    commit.AddKvlmField("tree", tree_sha);

    if (parent) {
      commit.AddKvlmField("parent", parent.value());
    }

    // "Name <email> 1527025023 +0200"
    std::string identity = author + " " + std::to_string(timestamp) + " " +
                           FormatTimezone(timestamp);

    commit.AddKvlmField("author", identity);
    commit.AddKvlmField("committer", identity);

    commit.SetKvlmMessage(Trim(message) + "\n");

    return ObjectWrite(commit, &repo);
  }

  std::string FormatTimezone(std::time_t timestamp) {
    std::tm time_info{};

    #ifdef _WIN32
      localtime_s(&time_info, &timestamp);
    #else
      localtime_r(&timestamp, &time_info);
    #endif

    // %z already produces git's format: +0200
    std::ostringstream oss;
    oss << std::put_time(&time_info, "%z");

    return oss.str();
  }

  std::string Trim(const std::string& str) {
    const char* whitespace = " \t\r\n";
    std::size_t start = str.find_first_not_of(whitespace);

    if (start == std::string::npos) {
      return "";
    }

    std::size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
  }
}
