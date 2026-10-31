#include "repository/LitRefs.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <map>

using std::filesystem::path;
using std::filesystem::is_regular_file;
using std::filesystem::is_directory;
using std::filesystem::directory_iterator;

namespace {

void RefListRecurse(LitRepository& repo,
                    std::map<std::string, std::string>& list,
                    path& ref_path);

}

std::optional<std::string> RefResolve(LitRepository& repo,
                                      const std::string& ref) {
  path ref_path = repo.RepoFile(false, ref);

  /*
    Sometimes an indirect reference may be broken. This is normal in a specific
    case: we are looking for HEAD on a new repository with no commits. In that
    case, .lit/HEAD points to "ref: refs/heads/main", but .lit/refs/heads/main
    don't exist yet.
  */
  if (!is_regular_file(ref_path)) {
    return std::nullopt;
  }

  std::ifstream ref_file(ref_path);
  if (!ref_file) {
    return std::nullopt;
  }

  std::string data;
  std::getline(ref_file, data);  // getline removes trailing \n

  if (data.rfind("ref: ", 0) == 0) {
    return RefResolve(repo, data.substr(5));
  }

  return data;
}

std::map<std::string, std::string> RefList(LitRepository& repo,
                                           std::string ref_dir) {
  std::map<std::string, std::string> result;
  path ref_dir_path = ref_dir;

  RefListRecurse(repo, result, ref_dir_path);
  return result;
}

void RefCreate(LitRepository& repo, std::string& ref_path, std::string& sha) {
  path ref_full_path = repo.RepoFile(true, ref_path);

  std::ofstream file(ref_full_path);
  if (!file) {
    throw std::runtime_error("Could not write a reference: " + 
      ref_full_path.string());
  }

  file << sha << '\n';
}

std::optional<std::string> BranchGetActive(LitRepository& repo) {
  std::ifstream head_file(repo.RepoPath("HEAD"));
  if (!head_file) {
    throw std::runtime_error("Could not read HEAD.");
  }

  std::string head;
  std::getline(head_file, head);

  const std::string prefix = "ref: refs/heads/";
  if (head.rfind(prefix, 0) == 0) {
    return head.substr(prefix.size());
  }

  return std::nullopt;
}

namespace {

  void RefListRecurse (LitRepository& repo,
                       std::map<std::string, std::string>& list,
                       path& ref_path) {
    path ref_dir = repo.RepoDir(false, ref_path);
  
    for (auto& entry : directory_iterator(ref_dir)) {
      path cur_ref_path = ref_path / entry.path().filename();
  
      if (entry.is_directory()) {
        RefListRecurse(repo, list, cur_ref_path);
      } else {
        auto resolved_opt = RefResolve(repo, cur_ref_path.string());

        if (resolved_opt) {
          list[cur_ref_path.string()] = resolved_opt.value();
        }
      }
    }
  }

}