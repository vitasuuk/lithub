#ifndef LIT_COMMANDS_H_
#define LIT_COMMANDS_H_

#include <filesystem>
#include <string>

int CmdInit(const std::filesystem::path& path);

int CmdCatFile(const std::string& format, const std::string& object);

int CmdHashObject(const std::string& format,
	              const std::filesystem::path& file_path,
	              bool write);

#endif  // LIT_COMMANDS_H_