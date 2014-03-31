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
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h"

#include <iostream>
#include <set>
#include <vector>
#include <string.h>

class StringSaver: public llvm::cl::StringSaver {
  std::set<std::string> Strings;
public:
  const char* SaveString(const char* s) {
    return this->Strings.insert(s).first->c_str();
  }
};

//----------------------------------------------------------------------------
int main(int argc_in, const char** argv_in)
{
  suppressInteractiveErrors();

  llvm::SmallVector<const char*, 64> argv;
  llvm::SpecificBumpPtrAllocator<char> argAlloc;
  if(llvm::error_code e =
     llvm::sys::Process::GetArgumentVector(
       argv, llvm::ArrayRef<const char*>(argv_in, argc_in), argAlloc)) {
    llvm::errs() << "error: could not get arguments: " << e.message() << "\n";
    return 1;
  } else if(argv.empty()) {
    llvm::errs() << "error: no argv[0]?!\n";
    return 1;
  }

  StringSaver argSaver;
  llvm::cl::ExpandResponseFiles(
    argSaver, llvm::cl::TokenizeGNUCommandLine, argv);

  size_t const argc = argv.size();

  if(!findResourceDir(argv[0], std::cerr)) {
    return 1;
  }

  const char* usage =
    "Usage: castxml ( <castxml-opt> | <clang-opt> | <src> )...\n"
    "\n"
    "  Options interpreted by castxml are listed below.\n"
    "  Remaining options are given to the internal Clang compiler.\n"
    "\n"
    "Options:\n"
    "\n"
    "  --castxml-cc-<id> <cc>\n"
    "  --castxml-cc-<id> \"(\" <cc> <cc-opt>... \")\"\n"
    "    Simulate given <id>-like compiler command, where <id> is\n"
    "    one of: gnu, msvc\n"
    "\n"
    "  --castxml-gccxml\n"
    "    Write gccxml-format output to <src>.xml or file named by '-o'\n"
    "\n"
    "  --castxml-start <name>\n"
    "    Start AST traversal at declaration with given (qualified) name\n"
    "\n"
    "  -help, --help\n"
    "    Print castxml and internal Clang compiler usage information\n"
    "\n"
    "  -o <file>\n"
    "    Write output to <file>\n"
    "\n"
    "  --version\n"
    "    Print castxml and internal Clang compiler version information\n"
    "\n"
    ;

  Options opts;
  llvm::SmallVector<const char *, 16> clang_args;
  llvm::SmallVector<const char *, 16> cc_args;
  const char* cc_id = 0;

  for(size_t i=1; i < argc; ++i) {
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
    } else if(strcmp(argv[i], "--castxml-start") == 0) {
      if((i+1) < argc) {
        opts.StartNames.push_back(argv[++i]);
      } else {
        std::cerr <<
          "error: argument to '--castxml-start' is missing "
          "(expected 1 value)\n"
          "\n" <<
          usage
          ;
        return 1;
      }
    } else if(strncmp(argv[i], "--castxml-cc-", 13) == 0) {
      if(!cc_id) {
        cc_id = argv[i] + 13;
        if((i+1) >= argc) {
          continue;
        }
        ++i;
        if(strncmp(argv[i], "-", 1) == 0) {
          std::cerr <<
            "error: argument to '--castxml-cc-" << cc_id <<
            "' may not start with '-'\n"
            "\n" <<
            usage
            ;
          return 1;
        }
        if(strcmp(argv[i], "(") == 0) {
          unsigned int depth = 1;
          for(++i; i < argc && depth > 0; ++i) {
            if(strncmp(argv[i], "--castxml-", 10) == 0) {
              std::cerr <<
                "error: arguments to '--castxml-cc-" << cc_id <<
                "' may not start with '--castxml-'\n"
                "\n" <<
                usage
                ;
              return 1;
            } else if(strcmp(argv[i], "(") == 0) {
              ++depth;
              cc_args.push_back(argv[i]);
            } else if(strcmp(argv[i], ")") == 0) {
              if(--depth) {
                cc_args.push_back(argv[i]);
              }
            } else {
              cc_args.push_back(argv[i]);
            }
          }
          if(depth) {
            std::cerr <<
              "error: unbalanced parentheses after '--castxml-cc-" <<
              cc_id << "'\n"
              "\n" <<
              usage
              ;
            return 1;
          }
          --i;
        } else {
          cc_args.push_back(argv[i]);
        }
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
    } else if(strcmp(argv[i], "-help") == 0 ||
              strcmp(argv[i], "--help") == 0) {
      std::cout <<
        usage <<
        "\n"
        "Help for the internal Clang compiler appears below.\n"
        "\n"
        "---------------------------------------------------------------"
        "\n" <<
        std::endl;
      // Also print Clang help.
      clang_args.push_back(argv[i]);
    } else if(strcmp(argv[i], "--version") == 0) {
      std::cout << "castxml version " << getVersionString() << std::endl;
      // Also print Clang version.
      clang_args.push_back(argv[i]);
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
