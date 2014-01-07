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
#ifndef CASTXML_UTILS_H
#define CASTXML_UTILS_H

#include <cxsys/Configure.hxx>
#include <string>

/// findResources - Call from main() to find resources
/// relative to the executable.  On success returns true.
/// On failure returns false and stores a message in the stream.
bool findResourceDir(const char* argv0, std::ostream& error);

/// getResourceDir - Get resource directory found at startup
std::string getResourceDir();

/// runCommand - Run a given command line and capture the output.
bool runCommand(int argc, const char* const* argv,
                int& ret, std::string& out, std::string& err,
                std::string& msg);

/// suppressInteractiveErrors - Disable Windows error dialog popups
void suppressInteractiveErrors();

#endif // CASTXML_UTILS_H
