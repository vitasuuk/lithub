/*
  Part of project resposible for writing and interracting with
  hash objects 
*/

#ifndef LIT_OBJECT_STORE_H_
#define LIT_OBJECT_STORE_H_

#include "LitObject.h"

#include <string>
#include <memory>
#include <istream>

class LitRepository;

std::string ObjectWrite(const LitObject& object,
                        LitRepository* repo = nullptr);

std::unique_ptr<LitObject> ObjectRead(LitRepository& repo,
                                      const std::string& sha);

std::string ObjectFind(LitRepository& repo, const std::string& name,
                       const std::string& format, bool follow = true);

std::string ObjectHash(std::istream& input, const std::string& format,
                       LitRepository* repo = nullptr);
		  
#endif  // LIT_OBJECT_STORE_H_
