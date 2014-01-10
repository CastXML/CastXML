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
#ifndef CASTXML_RUNCLANG_H
#define CASTXML_RUNCLANG_H

struct Options;

/// runClang - Run Clang with given user arguments and detected options.
int runClang(const char* const* argBeg,
             const char* const* argEnd,
             Options const& opts);

#endif // CASTXML_RUNCLANG_H
