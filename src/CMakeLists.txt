#=============================================================================
# Copyright Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

# Tell executables in the build tree where to find the source tree.
configure_file(
  "SourceDir.txt.in"
  "${CastXML_BINARY_DIR}/CMakeFiles/castxmlSourceDir.txt" @ONLY
  )
# Tell executables in the build tree where to find Clang resources.
configure_file(
  "ClangResourceDir.txt.in"
  "${CastXML_BINARY_DIR}/CMakeFiles/castxmlClangResourceDir.txt" @ONLY
  )
# Configure version number for C++ code.
configure_file(
  "Version.h.in"
  "${CastXML_BINARY_DIR}/src/Version.h" @ONLY
  )

set(clang_libs
  clangFrontend
  clangDriver
  clangSerialization
  clangParse
  clangSema
  clangAnalysis
  clangEdit
  clangAST
  clangLex
  clangBasic
  )

llvm_map_components_to_libraries(llvm_libs
  native
  option
  bitreader
  ${LLVM_TARGETS_TO_BUILD}
  )

add_executable(castxml
  castxml.cxx

  Detect.cxx Detect.h
  Options.h
  Output.cxx Output.h
  RunClang.cxx RunClang.h
  Utils.cxx Utils.h
  )
target_link_libraries(castxml
  cxsys
  ${clang_libs}
  ${llvm_libs}
  )
set_property(SOURCE Utils.cxx APPEND PROPERTY COMPILE_DEFINITIONS
  "CASTXML_INSTALL_DATA_DIR=\"${CastXML_INSTALL_DATA_DIR}\"")
install(TARGETS castxml DESTINATION ${CastXML_INSTALL_RUNTIME_DIR})
