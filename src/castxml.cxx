/*
  Copyright Kitware, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

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

#include "llvm/Config/llvm-config.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#if LLVM_VERSION_MAJOR >= 7
#  include "llvm/Support/InitLLVM.h"
#endif

#include <iostream>
#include <set>
#include <sstream>
#include <string.h>
#include <system_error>
#include <vector>
#include <map>

#if LLVM_VERSION_MAJOR > 3 ||                                                 \
  LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7
#  include "llvm/Support/StringSaver.h"
#else
class StringSaver : public llvm::cl::StringSaver
{
  std::set<std::string> Strings;

public:
  char const* SaveString(char const* s)
  {
    return this->Strings.insert(s).first->c_str();
  }
};
#endif

int main(int argc_in, char const** argv_in)
{
  suppressInteractiveErrors();

#if LLVM_VERSION_MAJOR >= 7
  llvm::InitLLVM initLLVM(argc_in, argv_in);
  llvm::SmallVector<char const*, 64> argv(argv_in, argv_in + argc_in);
#else
  llvm::SmallVector<char const*, 64> argv;
  llvm::SpecificBumpPtrAllocator<char> argAlloc;
  if (std::error_code e = llvm::sys::Process::GetArgumentVector(
        argv, llvm::ArrayRef<char const*>(argv_in, argc_in), argAlloc)) {
    llvm::errs() << "error: could not get arguments: " << e.message() << "\n";
    return 1;
  }
#endif

  if (argv.empty()) {
    llvm::errs() << "error: no argv[0]?!\n";
    return 1;
  }

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

#if LLVM_VERSION_MAJOR > 3 ||                                                 \
  LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 8
  llvm::BumpPtrAllocator argSaverAlloc;
  llvm::StringSaver argSaver(argSaverAlloc);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 7
  llvm::BumpPtrAllocator argSaverAlloc;
  llvm::BumpPtrStringSaver argSaver(argSaverAlloc);
#else
  StringSaver argSaver;
#endif
  llvm::cl::ExpandResponseFiles(argSaver, llvm::cl::TokenizeGNUCommandLine,
                                argv);

  size_t const argc = argv.size();

  if (!findResourceDir(argv[0], std::cerr)) {
    return 1;
  }

  /* clang-format off */
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
    "    Configure the internal Clang preprocessor and target\n"
    "    platform to match that of the given compiler command.\n"
    "    The <id> must be \"gnu\", \"msvc\", \"gnu-c\", or \"msvc-c\".\n"
    "    <cc> names a compiler (e.g. \"gcc\") and <cc-opt>... specifies\n"
    "    options that may affect its target (e.g. \"-m32\").\n"
    "\n"
    "  --castxml-output=<v>\n"
    "    Write castxml-format output to <src>.xml or file named by '-o'\n"
    "    The <v> specifies the \"epic\" format version number to generate,\n"
    "    and must be '1'.\n"
    "\n"
    "  --castxml-gccxml\n"
    "    Write gccxml-format output to <src>.xml or file named by '-o'\n"
    "    Use this option if you need an output similar to the one generated\n"
    "    by the legacy gccxml tool.\n"
    "\n"
    "  --castxml-start <name>[,<name>]...\n"
    "    Start AST traversal at declaration(s) with the given (qualified)\n"
    "    name(s).  Multiple names may be specified as a comma-separated\n"
    "    list or by repeating the option.\n"
    "\n"
    "  --castxml-exclude-by-visibility <specifier>[,<specifier>]...\n"
    "    Filter out declarations with the given visibility (private||protected||public)\n"
    "    specifier(s). Multiple specifiers may be provided as a comma-separated\n"
    "    list or by repeating the option.\n"
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
  /* clang-format on */

  Options opts;
  llvm::SmallVector<char const*, 16> clang_args;
  llvm::SmallVector<char const*, 16> cc_args;
  char const* cc_id = 0;

  typedef std::map<std::string, clang::AccessSpecifier> VisibilityMapType;
  VisibilityMapType TextToVisibility = {
    { "private",  clang::AccessSpecifier::AS_private },
    { "protected", clang::AccessSpecifier::AS_protected },
    { "public" , clang::AccessSpecifier::AS_public }
  };

  for (size_t i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--castxml-gccxml") == 0) {
      if (!opts.GccXml) {
        opts.GccXml = true;
      } else {
        /* clang-format off */
        std::cerr <<
          "error: '--castxml-gccxml' may be given at most once!\n"
          "\n" <<
          usage
          ;
        /* clang-format on */
        return 1;
      }
    } else if (strncmp(argv[i], "--castxml-output=", 17) == 0) {
      if (!opts.CastXml) {
        opts.CastXml = true;
        opts.CastXmlEpicFormatVersion = atoi(argv[i] + 17);
        if (opts.CastXmlEpicFormatVersion != 1) {
          /* clang-format off */
            std::cerr <<
              "error: '--castxml-output=<v>' accepts only '1' as '<v>'!\n"
              "\n" <<
              usage
              ;
          /* clang-format on */
        }
      } else {
        /* clang-format off */
        std::cerr <<
          "error: '--castxml-output=<v>' may be given at most once!\n"
          "\n" <<
          usage
          ;
        /* clang-format on */
        return 1;
      }
    } else if (strcmp(argv[i], "--castxml-start") == 0) {
      if ((i + 1) < argc) {
        std::string item;
        std::stringstream stream(argv[++i]);
        while (std::getline(stream, item, ',')) {
          opts.StartNames.push_back(item);
        }
      } else {
        /* clang-format off */
        std::cerr <<
          "error: argument to '--castxml-start' is missing "
          "(expected 1 value)\n"
          "\n" <<
          usage
          ;
        /* clang-format on */
        return 1;
      }
    } else if (strncmp(argv[i], "--castxml-cc-", 13) == 0) {
      if (!cc_id) {
        cc_id = argv[i] + 13;
        if ((i + 1) >= argc) {
          continue;
        }
        ++i;
        if (strncmp(argv[i], "-", 1) == 0) {
          /* clang-format off */
          std::cerr <<
            "error: argument to '--castxml-cc-" << cc_id <<
            "' may not start with '-'\n"
            "\n" <<
            usage
            ;
          /* clang-format on */
          return 1;
        }
        if (strcmp(argv[i], "(") == 0) {
          unsigned int depth = 1;
          for (++i; i < argc && depth > 0; ++i) {
            if (strncmp(argv[i], "--castxml-", 10) == 0) {
              /* clang-format off */
              std::cerr <<
                "error: arguments to '--castxml-cc-" << cc_id <<
                "' may not start with '--castxml-'\n"
                "\n" <<
                usage
                ;
              /* clang-format on */
              return 1;
            } else if (strcmp(argv[i], "(") == 0) {
              ++depth;
              cc_args.push_back(argv[i]);
            } else if (strcmp(argv[i], ")") == 0) {
              if (--depth) {
                cc_args.push_back(argv[i]);
              }
            } else {
              cc_args.push_back(argv[i]);
            }
          }
          if (depth) {
            /* clang-format off */
            std::cerr <<
              "error: unbalanced parentheses after '--castxml-cc-" <<
              cc_id << "'\n"
              "\n" <<
              usage
              ;
            /* clang-format on */
            return 1;
          }
          --i;
        } else {
          cc_args.push_back(argv[i]);
        }
      } else {
        /* clang-format off */
        std::cerr <<
          "error: '--castxml-cc-<id>' may be given at most once!\n"
          "\n" <<
          usage
          ;
        /* clang-format on */
        return 1;
      }
    } else if (strcmp(argv[i], "-E") == 0) {
      opts.PPOnly = true;
    } else if (strcmp(argv[i], "-o") == 0) {
      if ((i + 1) < argc) {
        opts.OutputFile = argv[++i];
      } else {
        /* clang-format off */
        std::cerr <<
          "error: argument to '-o' is missing (expected 1 value)\n"
          "\n" <<
          usage
          ;
        /* clang-format on */
        return 1;
      }
    }
    else if (strcmp(argv[i], "--castxml-exclude-by-visibility") == 0) {
      if ((i + 1) < argc) {
        std::string item;
        std::stringstream stream(argv[++i]);
        while (std::getline(stream, item, ',')) {
          VisibilityMapType::iterator iterator = TextToVisibility.find(item);
          if (iterator != TextToVisibility.end()) {
            opts.ExcludedVisibility.insert(iterator->second);
          }
        }
      }
      else {
        /* clang-format off */
        std::cerr <<
          "error: argument to '--castxml-exclude-by-visibility' is missing "
          "(expected 1 value)\n"
          "\n" <<
          usage
          ;
        /* clang-format on */
        return 1;
      }
    }
    else if (strcmp(argv[i], "-help") == 0 ||
      strcmp(argv[i], "--help") == 0) {
      /* clang-format off */
      std::cout <<
        usage <<
        "\n"
        "Help for the internal Clang compiler appears below.\n"
        "\n"
        "---------------------------------------------------------------"
        "\n" <<
        std::endl;
      /* clang-format on */
      // Also print Clang help.
      clang_args.push_back(argv[i]);
    } else if (strcmp(argv[i], "--version") == 0) {
      /* clang-format off */
      std::cout <<
        "castxml version " << getVersionString() << "\n"
        "\n"
        "CastXML project maintained and supported by Kitware "
        "(kitware.com).\n" <<
        std::endl;
      /* clang-format on */
      // Also print Clang version.
      clang_args.push_back(argv[i]);
    } else {
      clang_args.push_back(argv[i]);
      if (strcmp(argv[i], "-target") == 0 ||
          strcmp(argv[i], "--target") == 0 ||
          strncmp(argv[i], "-target=", 8) == 0 ||
          strncmp(argv[i], "--target=", 9) == 0) {
        opts.HaveTarget = true;
      } else if (strncmp(argv[i], "-std=", 5) == 0) {
        opts.HaveStd = true;
      }
    }
  }

  if (cc_id) {
    opts.HaveCC = true;
    if (cc_args.empty()) {
      /* clang-format off */
      std::cerr <<
        "error: '--castxml-cc-" << cc_id <<
        "' must be followed by a compiler command!\n"
        "\n" <<
        usage
        ;
      /* clang-format on */
      return 1;
    }
    if (!detectCC(cc_id, cc_args.data(), cc_args.data() + cc_args.size(),
                  opts)) {
      return 1;
    }
  }

  if (opts.GccXml && opts.CastXml) {
    /* clang-format off */
    std::cerr <<
      "error: '--castxml-gccxml' and '--castxml-output=<v>'"
      " may not be not be used together!\n"
      "\n" <<
      usage
      ;
    /* clang-format on */
    return 1;
  }

  if (clang_args.empty()) {
    return 0;
  }

  return runClang(clang_args.data(), clang_args.data() + clang_args.size(),
                  opts);
}
