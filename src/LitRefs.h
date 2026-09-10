#ifndef LIT_REFS_H_
#define LIT_REFS_H_

#include "LitRepository.h"

#include <filesystem>
#include <optional>
#include <string>
#include <map>

// Recursive reference resolver, which follows the reference files to find and
// return SHA-1 identifier.
// Returns nullopt in case the reference file doesn't exist yet.
std::optional<std::string> RefResolve(LitRepository& repo,
                                      const std::string& ref);
									

// Recursive reference gatherer, which follows specific reference path if
// such is given by user, otherwise gathers all references in .lit/refs/
// Returns a mapping of reference path to resolved sha1 hex encoding.
std::map<std::string, std::string> RefList(LitRepository& repo,
                                           std::string ref_dir = "refs");

#endif  // LIT_REFS_H_
