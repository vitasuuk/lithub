#ifndef LIT_COMMANDS_H_
#define LIT_COMMANDS_H_

#include <filesystem>
#include <string>

// lithub init - creates repository at given path
int CmdInit(const std::filesystem::path& path);

// lithub cat-file - Locates existing object by given format and SHA hashing,
// writes its raw payload to cout
int CmdCatFile(const std::string& format, const std::string& object);

// lithub hash-object - opens a regular file, constructs and hashes requested
// object type, optionally stores with -w, prints SHA hashing.
int CmdHashObject(const std::string& format,
	              const std::filesystem::path& file_path,
	              bool write);

// lithub log - walks commit ancestry and prints commit history.
int CmdLog(const std::string& commit_point);

#endif  // LIT_COMMANDS_H_