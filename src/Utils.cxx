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

#include "Utils.h"
#include "Version.h"

#include <fstream>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <system_error>
#include <vector>

static std::string castxmlResourceDir;
static std::string castxmlClangResourceDir;

static std::string GetMainExecutable(const char* argv0)
{
  return llvm::sys::fs::getMainExecutable(argv0,
                                          (void*)(intptr_t)GetMainExecutable);
}

static bool tryBuildDir(std::string const& dir)
{
  // Build tree has
  //   <build>/CMakeFiles/castxmlSourceDir.txt
  //   <build>/CMakeFiles/castxmlClangResourceDir.txt
  std::string src_dir_txt = dir + "/CMakeFiles/castxmlSourceDir.txt";
  std::string cl_dir_txt = dir + "/CMakeFiles/castxmlClangResourceDir.txt";
  std::ifstream src_fin(src_dir_txt.c_str());
  std::ifstream cl_fin(cl_dir_txt.c_str());
  std::string src_dir;
  std::string cl_dir;
  if (std::getline(src_fin, src_dir) && llvm::sys::fs::is_directory(src_dir) &&
      std::getline(cl_fin, cl_dir) && llvm::sys::fs::is_directory(cl_dir)) {
    castxmlResourceDir = src_dir + "/share/castxml";
    castxmlClangResourceDir = cl_dir;
    return true;
  }
  return false;
}

bool findResourceDir(const char* argv0, std::ostream& error)
{
  std::string exe = GetMainExecutable(argv0);
  if (!llvm::sys::path::is_absolute(exe)) {
    error << "error: unable to locate " << argv0 << "\n";
    return false;
  }

  // Install tree has
  //   <prefix>/bin/castxml
  //   <prefix>/<CASTXML_INSTALL_DATA_DIR>
  //   <prefix>/<CASTXML_INSTALL_DATA_DIR>/clang
  llvm::SmallString<16> dir(exe);
  llvm::sys::path::remove_filename(dir);
  llvm::sys::path::remove_filename(dir);
  castxmlResourceDir = std::string(dir.str()) + "/" + CASTXML_INSTALL_DATA_DIR;
  castxmlClangResourceDir = castxmlResourceDir + "/clang";
  if (!llvm::sys::fs::is_directory(castxmlResourceDir) ||
      !llvm::sys::fs::is_directory(castxmlClangResourceDir)) {
    llvm::SmallString<16> dir2(dir);
    llvm::sys::path::remove_filename(dir2);
    // Build tree has
    //   <build>/bin[/<config>]/castxml
    if (!tryBuildDir(dir.str()) && !tryBuildDir(dir2.str())) {
      error << "Unable to locate resources for " << exe << "\n";
      return false;
    }
  }

  return true;
}

std::string getResourceDir()
{
  return castxmlResourceDir;
}

std::string getClangResourceDir()
{
  return castxmlClangResourceDir;
}

std::string getVersionString()
{
  return CASTXML_VERSION_STRING;
}

unsigned int getVersionValue()
{
  return (CASTXML_VERSION_MAJOR * 1000000 + CASTXML_VERSION_MINOR * 1000 +
          CASTXML_VERSION_PATCH * 1);
}

bool runCommand(int argc, const char* const* argv, int& ret, std::string& out,
                std::string& err, std::string& msg)
{
  // Find the program to run.
  llvm::ErrorOr<std::string> maybeProg = llvm::sys::findProgramByName(argv[0]);
  if (std::error_code e = maybeProg.getError()) {
    msg = e.message();
    return false;
  }
  std::string const& prog = *maybeProg;

  // Create a temporary directory to hold output files.
  llvm::SmallString<128> tmpDir;
  if (std::error_code e =
        llvm::sys::fs::createUniqueDirectory("castxml", tmpDir)) {
    msg = e.message();
    return false;
  }
  llvm::SmallString<128> tmpOut = tmpDir;
  tmpOut.append("out");
  llvm::SmallString<128> tmpErr = tmpDir;
  tmpErr.append("err");

  // Construct file redirects.
  llvm::StringRef inFile; // empty means /dev/null
  llvm::StringRef outFile = tmpOut.str();
  llvm::StringRef errFile = tmpErr.str();
#if LLVM_VERSION_MAJOR >= 6
  llvm::Optional<llvm::StringRef> redirects[3];
  redirects[0] = inFile;
  redirects[1] = outFile;
  redirects[2] = errFile;
#else
  llvm::StringRef const* redirects[3];
  redirects[0] = &inFile;
  redirects[1] = &outFile;
  redirects[2] = &errFile;
#endif

  std::vector<const char*> cmd(argv, argv + argc);
  cmd.push_back(0);

  // Actually run the command.
  ret = llvm::sys::ExecuteAndWait(prog, &*cmd.begin(), nullptr, redirects, 0,
                                  0, &msg, nullptr);

  // Load the output from the temporary files.
  {
    std::ifstream fout(outFile.str());
    std::ifstream ferr(errFile.str());
    out.assign(std::istreambuf_iterator<char>(fout),
               std::istreambuf_iterator<char>());
    err.assign(std::istreambuf_iterator<char>(ferr),
               std::istreambuf_iterator<char>());
  }

  // Remove temporary files and directory.
  llvm::sys::fs::remove(llvm::Twine(tmpOut));
  llvm::sys::fs::remove(llvm::Twine(tmpErr));
  llvm::sys::fs::remove(llvm::Twine(tmpDir));

  return ret >= 0;
}

std::string encodeXML(std::string const& in, bool cdata)
{
  std::string xml;
  const char* last = in.c_str();
  for (const char* c = last; *c; ++c) {
    switch (*c) {
#define XML(OUT)                                                              \
  xml.append(last, c - last);                                                 \
  last = c + 1;                                                               \
  xml.append(OUT)
      case '&':
        XML("&amp;");
        break;
      case '<':
        XML("&lt;");
        break;
      case '>':
        XML("&gt;");
        break;
      case '\'':
        if (!cdata) {
          XML("&apos;");
        }
        break;
      case '"':
        if (!cdata) {
          XML("&quot;");
        }
        break;
      default:
        break;
#undef XML
    }
  }
  xml.append(last);
  return xml;
}

std::string stringReplace(std::string str, std::string const& in,
                          std::string const& out)
{
  std::string::size_type p = 0;
  while ((p = str.find(in, p)) != std::string::npos) {
    str.replace(p, in.size(), out);
    p += out.length();
  }
  return str;
}

#if defined(_WIN32)
#include <windows.h>
#endif

void suppressInteractiveErrors()
{
#if defined(_WIN32)
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}
