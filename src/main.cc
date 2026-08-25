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

	  } else if (command == "cat-file") {
      if (argc != 4) {
        std::cerr << "usage: lithub cat-file <type> <object>\n";
        return 1;
      }

      std::string type = argv[2];
      std::string object = argv [3];

      if (type != "blob" && type != "commit" &&
          type != "tree" && type != "tag") {
        std::cerr << "Invalid object type: " + type << '\n';
        return 1;
	    }

      return CmdCatFile(type, object);

	  } else if (command == "hash-object") {
      bool write = false;
      std::string format = "blob";
      path file_path;
      bool has_file = false;

      for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-w") {
          write = true;
        } else if (arg == "-t") {
          if (i + 1 >= argc) {
            std::cerr << "usage: lithub hash-object [-w] [-t TYPE] FILE\n";
            return 1;
          }

          format = argv[++i];
        } else if (!has_file) {
          file_path = arg;
          has_file = true;
        } else {
          std::cerr << "usage: lithub hash-object [-w] [-t TYPE] FILE\n";
          return 1;
        }
      }

      if(!has_file) {
        std::cerr << "usage: lithub hash-object [-w] [-t TYPE] FILE\n";
        return 1;
      }

      return CmdHashObject(format, file_path, write);
    }

    std::cerr << "Unknown command: " << command << '\n';
    return 1;
  } catch (const std::exception& e) {
	  std::cerr << e.what() << '\n';
    return 1;
  }
}