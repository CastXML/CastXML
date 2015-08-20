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

#include <cxsys/Process.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <fstream>
#include <vector>

static std::string castxmlResourceDir;
static std::string castxmlClangResourceDir;

//----------------------------------------------------------------------------
static std::string GetMainExecutable(const char* argv0)
{
  return llvm::sys::fs::getMainExecutable
    (argv0, (void*)(intptr_t)GetMainExecutable);
}

//----------------------------------------------------------------------------
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
  if (std::getline(src_fin, src_dir) &&
      llvm::sys::fs::is_directory(src_dir) &&
      std::getline(cl_fin, cl_dir) &&
      llvm::sys::fs::is_directory(cl_dir)) {
    castxmlResourceDir = src_dir + "/share/castxml";
    castxmlClangResourceDir = cl_dir;
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
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
    if (!tryBuildDir(dir.str()) &&
        !tryBuildDir(dir2.str())) {
      error << "Unable to locate resources for " << exe << "\n";
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
std::string getResourceDir()
{
  return castxmlResourceDir;
}

//----------------------------------------------------------------------------
std::string getClangResourceDir()
{
  return castxmlClangResourceDir;
}

//----------------------------------------------------------------------------
std::string getVersionString()
{
  return CASTXML_VERSION;
}

//----------------------------------------------------------------------------
bool runCommand(int argc, const char* const* argv,
                int& ret, std::string& out, std::string& err,
                std::string& msg)
{
  std::vector<const char*> cmd(argv, argv + argc);
  cmd.push_back(0);
  ret = 1;
  out = "";
  err = "";
  std::vector<char> outBuf;
  std::vector<char> errBuf;

  cxsysProcess* cp = cxsysProcess_New();
  cxsysProcess_SetCommand(cp, &*cmd.begin());
  cxsysProcess_SetOption(cp, cxsysProcess_Option_HideWindow, 1);
#ifdef _WIN32
  cxsysProcess_SetPipeFile(cp, cxsysProcess_Pipe_STDIN, "//./nul");
#else
  cxsysProcess_SetPipeFile(cp, cxsysProcess_Pipe_STDIN, "/dev/null");
#endif
  cxsysProcess_Execute(cp);

  char* data;
  int length;
  int pipe;
  while((pipe = cxsysProcess_WaitForData(cp, &data, &length, 0)) > 0) {
    if(pipe == cxsysProcess_Pipe_STDOUT) {
      outBuf.insert(outBuf.end(), data, data+length);
    } else if(pipe == cxsysProcess_Pipe_STDERR) {
      errBuf.insert(errBuf.end(), data, data+length);
    }
  }

  cxsysProcess_WaitForExit(cp, 0);
  if(!outBuf.empty()) {
    out.append(&*outBuf.begin(), outBuf.size());
  }
  if(!errBuf.empty()) {
    err.append(&*errBuf.begin(), errBuf.size());
  }

  bool result = true;
  switch(cxsysProcess_GetState(cp)) {
  case cxsysProcess_State_Exited:
    ret = cxsysProcess_GetExitValue(cp);
    break;
  case cxsysProcess_State_Exception:
    msg = cxsysProcess_GetExceptionString(cp);
    result = false;
    break;
  case cxsysProcess_State_Error:
    msg = cxsysProcess_GetErrorString(cp);
    result = false;
    break;
  default:
    msg = "Process terminated in unexpected state.\n";
    result = false;
    break;
  }

  cxsysProcess_Delete(cp);
  return result;
}

//----------------------------------------------------------------------------
std::string encodeXML(std::string const& in, bool cdata)
{
  std::string xml;
  const char* last = in.c_str();
  for(const char* c = last; *c; ++c) {
    switch(*c) {
#   define XML(OUT)               \
      xml.append(last, c - last); \
      last = c + 1;               \
      xml.append(OUT)
    case '&': XML("&amp;"); break;
    case '<': XML("&lt;"); break;
    case '>': XML("&gt;"); break;
    case '\'':
      if(!cdata) {
        XML("&apos;");
      }
      break;
    case '"':
      if(!cdata) {
        XML("&quot;");
      }
      break;
    default: break;
#   undef XML
    }
  }
  xml.append(last);
  return xml;
}

#if defined(_WIN32)
# include <windows.h>
#endif

//----------------------------------------------------------------------------
void suppressInteractiveErrors()
{
#if defined(_WIN32)
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}
