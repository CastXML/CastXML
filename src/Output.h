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
#ifndef CASTXML_OUTPUT_H
#define CASTXML_OUTPUT_H

#include <cxsys/Configure.h>

namespace llvm {
class raw_ostream;
}

namespace clang {
class CompilerInstance;
class ASTContext;
}

struct Options;

/// outputXML - Print a gccxml-compatible AST dump.
void outputXML(clang::CompilerInstance& ci, clang::ASTContext& ctx,
               llvm::raw_ostream& os, Options const& opts);

#endif // CASTXML_OUTPUT_H
