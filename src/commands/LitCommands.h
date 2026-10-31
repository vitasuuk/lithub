#ifndef LIT_COMMANDS_H_
#define LIT_COMMANDS_H_

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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
int CmdLog(const std::string& commit_point);

// lithub ls-tree - prints contents of the tree, optionally recursively.
int CmdLsTree(const std::string& tree_point, bool recurse);

// lithub checkout - materializes the snapshot represented by commit (its files
// and directories) in the filesystem. Passed directory should be EMPTY:
//   result of simplification, git has complex data preservation / replacement
//   mechanism that deletes / replaces only what is necessary !!!
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

// Command to solve references. With optional --lithub--type argument
// looks for the SHA of given object type by following objects like tags/commits
int CmdRevParse(std::string type, std::string name);

// Displays the names of files in the staging area.
// Additionally displays detailed info if --verbose flag is used
int CmdLsFiles(bool verbose);

// lithub status - prints active branch, then changes between HEAD and the
// index (staged), and between the index and the worktree (unstaged and
// untracked files).
// Note: ignore rules are not applied yet, every untracked file is listed.
int CmdStatus();

// lithub rm - removes files from the index AND deletes them from the
// worktree. Unlike git, doesn't check whether the files have unsaved
// changes before deleting.
int CmdRm(const std::vector<std::string>& paths);

// lithub add - hashes files into blobs and stages them in the index,
// replacing existing entries for the same paths.
int CmdAdd(const std::vector<std::string>& paths);

// lithub commit - builds trees from the index, creates a commit object
// on top of HEAD and moves the active branch (or detached HEAD) to it.
// Author identity is read from git's global config (user.name, user.email).
int CmdCommit(const std::string& message);

#endif  // LIT_COMMANDS_H_
