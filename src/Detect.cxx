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
#include "Utils.h"

#include <cxsys/SystemTools.hxx>

#include <iostream>
#include <string.h>

//----------------------------------------------------------------------------
static bool failedCC(const char* id,
                     std::vector<const char*> const& args,
                     std::string const& out,
                     std::string const& err,
                     std::string const& msg)
{
  std::cerr << "error: '--castxml-cc-" << id
            << "' compiler command failed:\n\n";
  for(std::vector<const char*>::const_iterator i = args.begin(),
        e = args.end(); i != e; ++i) {
    std::cerr << " '" << *i << "'";
  }
  std::cerr << "\n";
  if(!msg.empty()) {
    std::cerr << msg << "\n";
  } else {
    std::cerr << out << "\n";
    std::cerr << err << "\n";
  }
  return false;
}

//----------------------------------------------------------------------------
static void setTriple(Options& opts)
{
  std::string const& pd = opts.Predefines;
  std::string arch;
  std::string os;
  if(pd.find("#define __x86_64__ 1") != pd.npos ||
     pd.find("#define _M_X64 ") != pd.npos) {
    arch = "x86_64";
  } else if(pd.find("#define __amd64__ 1") != pd.npos ||
            pd.find("#define _M_AMD64 ") != pd.npos) {
    arch = "amd64";
  } else if(pd.find("#define __i386__ 1") != pd.npos ||
            pd.find("#define _M_IX86 ") != pd.npos) {
    arch = "i386";
  }
  if(pd.find("#define __MINGW32__ 1") != pd.npos) {
    if(arch.find("64") != arch.npos) {
      os = "w64-mingw32";
    } else {
      os = "pc-mingw32";
    }
  } else if(pd.find("#define _WIN32 1") != pd.npos) {
    os = "pc-win32";
  }
  if(!arch.empty() && !os.empty()) {
    opts.Triple = arch + "-" + os;
  }
}

//----------------------------------------------------------------------------
static bool detectCC_GNU(const char* const* argBeg,
                         const char* const* argEnd,
                         Options& opts)
{
  std::string const fwExplicitSuffix = " (framework directory)";
  std::string const fwImplicitSuffix = "/Frameworks";
  std::vector<const char*> cc_args(argBeg, argEnd);
  std::string empty_cpp = getResourceDir() + "/empty.cpp";
  int ret;
  std::string out;
  std::string err;
  std::string msg;
  cc_args.push_back("-E");
  cc_args.push_back("-dM");
  cc_args.push_back("-v");
  cc_args.push_back(empty_cpp.c_str());
  if(runCommand(int(cc_args.size()), &cc_args[0], ret, out, err, msg) &&
     ret == 0) {
    opts.Predefines = out;
    const char* start_line = "#include <...> search starts here:";
    if(const char* c = strstr(err.c_str(), start_line)) {
      if((c = strchr(c, '\n'), c++)) {
        while(*c++ == ' ') {
          if(const char* e = strchr(c, '\n')) {
            const char* s = c;
            c = e + 1;
            if(*(e - 1) == '\r') {
              --e;
            }
            std::string inc(s, e-s);
            cxsys::SystemTools::ConvertToUnixSlashes(inc);
            bool fw = ((inc.size() > fwExplicitSuffix.size()) &&
                       (inc.substr(inc.size()-fwExplicitSuffix.size()) ==
                        fwExplicitSuffix));
            if(fw) {
              inc = inc.substr(0, inc.size()-fwExplicitSuffix.size());
            } else {
              fw = ((inc.size() > fwImplicitSuffix.size()) &&
                    (inc.substr(inc.size()-fwImplicitSuffix.size()) ==
                     fwImplicitSuffix));
            }
            opts.Includes.push_back(Options::Include(inc, fw));
          }
        }
      }
    }
    setTriple(opts);
    return true;
  } else {
    return failedCC("gnu", cc_args, out, err, msg);
  }
}

//----------------------------------------------------------------------------
static bool detectCC_MSVC(const char* const* argBeg,
                          const char* const* argEnd,
                          Options& opts)
{
  std::vector<const char*> cc_args(argBeg, argEnd);
  std::string detect_vs_cpp = getResourceDir() + "/detect_vs.cpp";
  int ret;
  std::string out;
  std::string err;
  std::string msg;
  cc_args.push_back("-c");
  cc_args.push_back("-FoNUL");
  cc_args.push_back(detect_vs_cpp.c_str());
  if(runCommand(int(cc_args.size()), &cc_args[0], ret, out, err, msg) &&
     ret == 0) {
    if(const char* predefs = strstr(out.c_str(), "\n#define")) {
      opts.Predefines = predefs+1;
    }
    if(const char* includes_str = cxsys::SystemTools::GetEnv("INCLUDE")) {
      std::vector<std::string> includes;
      cxsys::SystemTools::Split(includes_str, includes, ';');
      for(std::vector<std::string>::iterator i = includes.begin(),
            e = includes.end(); i != e; ++i) {
        if(!i->empty()) {
          std::string inc = *i;
          cxsys::SystemTools::ConvertToUnixSlashes(inc);
          opts.Includes.push_back(inc);
        }
      }
    }
    setTriple(opts);
    return true;
  } else {
    return failedCC("msvc", cc_args, out, err, msg);
  }
}

//----------------------------------------------------------------------------
bool detectCC(const char* id,
              const char* const* argBeg,
              const char* const* argEnd,
              Options& opts)
{
  if(strcmp(id, "gnu") == 0) {
    return detectCC_GNU(argBeg, argEnd, opts);
  } else if(strcmp(id, "msvc") == 0) {
    return detectCC_MSVC(argBeg, argEnd, opts);
  } else {
    std::cerr << "error: '--castxml-cc-" << id << "' not known!\n";
    return false;
  }
}
