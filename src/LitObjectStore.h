/*
  Part of project resposible for writing and interracting with
  hash objects 
*/

#ifndef LIT_OBJECT_STORE_H_
#define LIT_OBJECT_STORE_H_

#include "LitObject.h"

#include <memory>
#include <string>

class LitRepository;

std::string ObjectWrite(const LitObject& object,
                        LitRepository* repo = nullptr);

std::unique_ptr<LitObject> ObjectRead(LitRepository& repo,
                                      const std::string& sha);

#endif  // LIT_OBJECT_STORE_H_