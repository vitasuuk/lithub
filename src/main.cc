#include "LitCommands.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

using std::filesystem::path;

constexpr char kUsageMessage[] = "usage: lithub <command> [args]\n";

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << kUsageMessage;
    return 1;
  }

  std::string command = argv[1];

  try {
    if (command == "init") {
      if (argc > 3) {
        std::cerr << "usage: lithub init [directory]\n";
        return 1;
      }

      path repo_path = ".";

      if (argc == 3) {
        repo_path = argv[2];
      }

      return CmdInit(repo_path);
	  }

    std::cerr << "Unknown command: " << command << '\n';
  } catch (const std::exception& e) {
	  std::cerr << e.what() << '\n';
    return 1;
  }
}