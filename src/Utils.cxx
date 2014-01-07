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

#include <cxsys/Process.h>
#include <cxsys/SystemTools.hxx>
#include <llvm/Support/FileSystem.h>
#include <fstream>
#include <vector>

static std::string castxmlResourceDir;

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
  std::string src_dir_txt = dir + "/CMakeFiles/castxmlSourceDir.txt";
  std::ifstream src_fin(src_dir_txt.c_str());
  std::string src_dir;
  if(src_fin && cxsys::SystemTools::GetLineFromStream(src_fin, src_dir) &&
     cxsys::SystemTools::FileIsDirectory(src_dir.c_str())) {
    castxmlResourceDir = src_dir + "/share/castxml";
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool findResourceDir(const char* argv0, std::ostream& error)
{
  std::string exe = GetMainExecutable(argv0);
  if(!cxsys::SystemTools::FileIsFullPath(exe.c_str())) {
    error << "error: unable to locate " << argv0 << "\n";
    return false;
  }
  std::string exe_dir = cxsys::SystemTools::GetFilenamePath(exe);

  // Install tree has
  //   <prefix>/bin/castxml
  //   <prefix>/<CASTXML_INSTALL_DATA_DIR>
  std::string dir = cxsys::SystemTools::GetFilenamePath(exe_dir);
  castxmlResourceDir = dir + "/" + CASTXML_INSTALL_DATA_DIR;
  if(!cxsys::SystemTools::FileIsDirectory(castxmlResourceDir.c_str())) {
    // Build tree has
    //   <build>/bin[/<config>]/castxml
    if(!tryBuildDir(dir) &&
       !tryBuildDir(cxsys::SystemTools::GetFilenamePath(dir))) {
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
