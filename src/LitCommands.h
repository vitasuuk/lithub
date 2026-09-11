#ifndef LIT_COMMANDS_H_
#define LIT_COMMANDS_H_

#include <filesystem>
#include <string>

// lithub init - creates repository at given path
int CmdInit(const std::filesystem::path& path);

// lithub cat-file - Locates existing object by given format and reference,
// writes its raw payload to cout
int CmdCatFile(const std::string& format, const std::string& object);

// lithub hash-object - opens a regular file, constructs and hashes requested
// object type, optionally stores with -w, prints SHA hashing.
int CmdHashObject(const std::string& format,
	              const std::filesystem::path& file_path,
	              bool write);

// lithub log - walks commit ancestry and prints commit history.
// Temporarily accepts only commit sha, no reference support so far.
int CmdLog(const std::string& commit_point);

// lithub ls-tree - prints contents of the tree, optionally recursively.
// Temporarily accepts only tree sha, no reference support so far.
int CmdLsTree(const std::string& tree_point, bool recurse);

// lithub checkout - materializes the snapshot represented by commit (its files
// and directories) in the filesystem. Passed directory should be EMPTY:
//   result of simplification, git has complex data preservation / replacement
//   mechanism that deletes / replaces only what is necessary !!!
// Temporarily accepts only commit sha, no reference support so far.
int CmdCheckout(const std::string& commit_point, 
                const std::filesystem::path& dir_path);

// lithub show-ref - accepts reference directory path relative to .lit,
// recursively iterates given directory and all references paths and their
// resolved SHA1.
// If no directory path is given, lists all references in .lit/refs by default;
int CmdRefList(const std::string& ref_dir);

// lithub tag - lists tags if no name and object provided, otherwise creates
// a lighweight/full tag (-a flag) to passed reference/commit sha (or HEAD by
// default) using given by user name.
int CmdTag(bool create_tag_obj,
	         std::optional<std::string> name,
           std::string object);

#endif  // LIT_COMMANDS_H_
