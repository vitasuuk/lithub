#include "LitCommands.h"

#include <argparse/argparse.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

using std::filesystem::path;

constexpr char kUsageMessage[] = "usage: lithub <command> [args]\n";

int main(int argc, char* argv[]) {
  argparse::ArgumentParser program("lithub");

  // init
  argparse::ArgumentParser init_command("init");
  init_command.add_argument("directory")
    .nargs(argparse::nargs_pattern::optional)
    .default_value(std::string("."));

  // cat-file
  argparse::ArgumentParser cat_file_command("cat-file");
  cat_file_command.add_argument("type")
    .choices("blob", "commit", "tree", "tag");
  cat_file_command.add_argument("object");

  // hash-object
  argparse::ArgumentParser hash_object_command("hash-object");
  hash_object_command.add_argument("-w")
    .default_value(false)
    .implicit_value(true);

  hash_object_command.add_argument("-t")
    .default_value(std::string("blob"))
    .choices("blob", "commit", "tree", "tag");

  hash_object_command.add_argument("file");

  // log
  argparse::ArgumentParser log_command("log");
  log_command.add_argument("commit");

  // ls-tree
  argparse::ArgumentParser ls_tree_command("ls-tree");
  ls_tree_command.add_argument("-r")
    .default_value(false)
    .implicit_value(true);
  ls_tree_command.add_argument("tree");

  // checkout
  argparse::ArgumentParser checkout_command("checkout");
  checkout_command.add_argument("commit");
  checkout_command.add_argument("dir_path");

  // show-ref
  argparse::ArgumentParser show_ref_command("show-ref");
  show_ref_command.add_argument("ref_dir")
    .nargs(argparse::nargs_pattern::optional)
    .default_value("refs");
  
  program.add_subparser(init_command);
  program.add_subparser(cat_file_command);
  program.add_subparser(hash_object_command);
  program.add_subparser(log_command);
  program.add_subparser(ls_tree_command);
  program.add_subparser(checkout_command);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n' << program;
    return EXIT_FAILURE;
  }

  try {
    if (program.is_subcommand_used("init")) {
      path repo_path = init_command.get<std::string>("directory");

      return CmdInit(repo_path);
    }

    if (program.is_subcommand_used("cat-file")) {
      std::string format = cat_file_command.get<std::string>("type");
      std::string object = cat_file_command.get<std::string>("object");

      return CmdCatFile(format, object);
    }

    if (program.is_subcommand_used("hash-object")) {
      bool write = hash_object_command.get<bool>("-w");
      std::string format = hash_object_command.get<std::string>("-t");
      path file_path = hash_object_command.get<std::string>("file");

      return CmdHashObject(format, file_path, write);
    }

	  if (program.is_subcommand_used("ls-tree")) {
      bool recurse = ls_tree_command.get<bool>("-r");
      std::string tree = ls_tree_command.get<std::string>("tree");

      return CmdLsTree(tree, recurse);
	  }

    if (program.is_subcommand_used("checkout")) {
      std::string commit = checkout_command.get<std::string>("commit");
      path dir_path = checkout_command.get<std::string>("dir_path");

      return CmdCheckout(commit, dir_path);
    }

	  if (program.is_subcommand_used("show-ref")) {
	    std::string ref_dir = show_ref_command.get<std::string>("ref_dir");

      
	  }

    std::cerr << program;
    return EXIT_FAILURE;

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';

    return EXIT_FAILURE;
  }
}
