#ifndef LIT_COMMANDS_H_
#define LIT_COMMANDS_H_

#include <filesystem>
#include <string>

int CmdInit(const std::filesystem::path& path);

int CmdCatFile(const std::string& type, const std::string& object);

int CmdHashObject(const std::string& format, const path& path,
	              bool store = false);

#endif  // LIT_COMMANDS_H_