#include "LitCommands.h"
#include "LitRepository.h"

using std::filesystem::path;

int CmdInit(const path& path) {
  LitRepository::RepoCreate(path);
  return 0;
}
