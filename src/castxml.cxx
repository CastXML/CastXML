/*
  Copyright Kitware, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "Options.h"
#include "Utils.h"

#include "llvm/ADT/SmallVector.h"

#include <cxsys/Encoding.hxx>
#include <iostream>
#include <vector>
#include <string.h>

//----------------------------------------------------------------------------
int main(int argc, const char* const * argv)
{
  suppressInteractiveErrors();
  cxsys::Encoding::CommandLineArguments args =
    cxsys::Encoding::CommandLineArguments::Main(argc, argv);
  argc = args.argc();
  argv = args.argv();
  if(!findResourceDir(argv[0], std::cerr)) {
    return 1;
  }

  const char* usage =
    "Usage: castxml [--castxml-gccxml] [<clang-args>...]\n"
    ;

  Options opts;
  llvm::SmallVector<const char *, 16> clang_args;

  for(int i=1; i < argc; ++i) {
    if(strcmp(argv[i], "--castxml-gccxml") == 0) {
      if(!opts.GccXml) {
        opts.GccXml = true;
      } else {
        std::cerr <<
          "error: '--castxml-gccxml' may be given at most once!\n"
          "\n" <<
          usage
          ;
        return 1;
      }
    } else {
      clang_args.push_back(argv[i]);
    }
  }

  return 0;
}
