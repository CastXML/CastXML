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

#include "Detect.h"
#include "Options.h"
#include "RunClang.h"
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
    "Usage: castxml [--castxml-gccxml] [<clang-args>...] \\\n"
    "               [--castxml-cc-<id> <cc> [<cc-args>...]]\n"
    ;

  Options opts;
  llvm::SmallVector<const char *, 16> clang_args;
  llvm::SmallVector<const char *, 16> cc_args;
  const char* cc_id = 0;

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
    } else if(strncmp(argv[i], "--castxml-cc-", 13) == 0) {
      if(!cc_id) {
        cc_id = argv[i] + 13;
        for(++i;i < argc && strncmp(argv[i], "--castxml-", 10) != 0; ++i) {
          cc_args.push_back(argv[i]);
        }
        --i;
      } else {
        std::cerr <<
          "error: '--castxml-cc-<id>' may be given at most once!\n"
          "\n" <<
          usage
          ;
        return 1;
      }
    } else if(strcmp(argv[i], "-E") == 0) {
      opts.PPOnly = true;
    } else if(strcmp(argv[i], "-o") == 0) {
      if((i+1) < argc) {
        opts.OutputFile = argv[++i];
      } else {
        std::cerr <<
          "error: argument to '-o' is missing (expected 1 value)\n"
          "\n" <<
          usage
          ;
        return 1;
      }
    } else {
      clang_args.push_back(argv[i]);
    }
  }

  if(opts.PPOnly && opts.GccXml) {
    std::cerr <<
      "error: '--castxml-gccxml' and '-E' may not both be given\n"
      "\n" <<
      usage
      ;
    return 1;
  }

  if(cc_id) {
    opts.HaveCC = true;
    if(cc_args.empty()) {
      std::cerr <<
        "error: '--castxml-cc-" << cc_id <<
        "' must be followed by a compiler command!\n"
        "\n" <<
        usage
        ;
      return 1;
    }
    if(!detectCC(cc_id, cc_args.data(), cc_args.data() + cc_args.size(),
                 opts)) {
      return 1;
    }
  }

  if(clang_args.empty()) {
    return 0;
  }

  return runClang(clang_args.data(), clang_args.data() + clang_args.size(),
                  opts);
}
