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
cmake_minimum_required(VERSION 2.8.5)
project(CastXML)

include(src/Version.cmake)

include(CTest)

# Build tree locations.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CastXML_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CastXML_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CastXML_BINARY_DIR}/lib")

# Install tree locations.
if(NOT CastXML_INSTALL_RUNTIME_DIR)
  set(CastXML_INSTALL_RUNTIME_DIR bin)
endif()
if(NOT CastXML_INSTALL_DATA_DIR)
  set(CastXML_INSTALL_DATA_DIR share/castxml)
endif()
if(NOT CastXML_INSTALL_DOC_DIR)
  set(CastXML_INSTALL_DOC_DIR share/doc/castxml)
endif()
if(NOT CastXML_INSTALL_MAN_DIR)
  set(CastXML_INSTALL_MAN_DIR man)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  if(NOT LLVM_ENABLE_EH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  endif()
  if(NOT LLVM_ENABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  endif()
  if(CYGWIN OR MINGW)
    # Use GNU extensions on Windows as LLVM upstream does.
    set(std_cxx11 "-std=gnu++11")
  else()
    set(std_cxx11 "-std=c++11")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${std_cxx11}")
endif()

macro(hint_clang_from_llvm)
  if(LLVM_DIR AND NOT Clang_DIR)
    if(LLVM_DIR MATCHES "share/llvm/cmake$")
      # LLVM/Clang 3.8 installs in <prefix>/share/{llvm,clang}/cmake
      get_filename_component(Clang_DIR "${LLVM_DIR}/../../clang/cmake" ABSOLUTE)
    else()
      # LLVM/Clang 3.9+ installs in <prefix>/lib/cmake/{llvm,clang}
      get_filename_component(Clang_DIR "${LLVM_DIR}/../clang" ABSOLUTE)
    endif()
  endif()
endmacro()

macro(hint_llvm_from_clang)
  if(Clang_DIR AND NOT LLVM_DIR)
    if(Clang_DIR MATCHES "share/clang/cmake$")
      # LLVM/Clang 3.8 installs in <prefix>/share/{llvm,clang}/cmake
      get_filename_component(LLVM_DIR "${Clang_DIR}/../../llvm/cmake" ABSOLUTE)
    else()
      # LLVM/Clang 3.9+ installs in <prefix>/lib/cmake/{llvm,clang}
      get_filename_component(LLVM_DIR "${Clang_DIR}/../llvm" ABSOLUTE)
    endif()
  endif()
endmacro()

# Build scripts may set either LLVM_DIR or Clang_DIR.
hint_llvm_from_clang()
hint_clang_from_llvm()

# Clang 3.8+ installs its own CMake package.  Try it first.
find_package(Clang QUIET)

# If Clang did not find LLVM for us, give our search below a hint.
hint_llvm_from_clang()

# Clang 3.9+ automatically searches for LLVM.  For older versions we
# may need to search for LLVM directly.  Either way we require LLVM.
if(NOT LLVM_FOUND)
  find_package(LLVM REQUIRED)
endif()

if(DEFINED LLVM_BUILD_BINARY_DIR)
  message(FATAL_ERROR
    "Clang_DIR or LLVM_DIR refers to a LLVM/Clang build directory:\n"
    "  Clang_DIR=${Clang_DIR}\n"
    "  LLVM_DIR=${LLVM_DIR}\n"
    "CastXML must be built against a LLVM/Clang install tree as "
    "specified in\n"
    "  ${CastXML_SOURCE_DIR}/README.rst")
endif()

set(found_clang 0)
set(tried_clang)
foreach(inc ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
  if(EXISTS "${inc}/clang/AST/ASTConsumer.h")
    set(found_clang 1)
    break()
  else()
    set(tried_clang "  ${inc}\n")
  endif()
endforeach()
if(NOT found_clang)
  message(FATAL_ERROR
    "Clang_DIR or LLVM_DIR refers to a LLVM/Clang installation:\n"
    "  Clang_DIR=${Clang_DIR}\n"
    "  LLVM_DIR=${LLVM_DIR}\n"
    "that does not have `clang/` headers in any include directory:\n"
    "${tried_clang}"
    )
endif()

add_definitions(${LLVM_DEFINITIONS})
include_directories(${CLANG_INCLUDE_DIRS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
if(NOT DEFINED LLVM_VERSION_PATCH)
  set(LLVM_VERSION_PATCH 0)
endif()

set(CLANG_RESOURCE_DIR "" CACHE PATH "Clang resource directory")
if(NOT CLANG_RESOURCE_DIR)
  set(v ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH})
  set(tried "")
  foreach(d ${LLVM_LIBRARY_DIRS})
    if(IS_DIRECTORY "${d}/clang/${v}/include")
      set(CLANG_RESOURCE_DIR ${d}/clang/${v})
      break()
    endif()
    set(tried "${tried}\n  ${d}/clang/${v}")
    
    # On Gentoo clang resources are installed side-by-side with llvm: 
    #   /usr/lib64/llvm/...
    #   /usr/lib64/clang/... 
    # Extract the base library directory from the LLVM library directory
    # and check whether the clang resources are there
    string(REGEX REPLACE  "llvm.*" "" dd ${d})
    if(IS_DIRECTORY "${dd}/clang/${v}/include")
      set(CLANG_RESOURCE_DIR ${dd}/clang/${v})
      break()
    endif()
    set(tried "${tried}\n  ${dd}/clang/${v}")

  endforeach()
  if(NOT CLANG_RESOURCE_DIR)
    if(tried)
      set(tried "  Tried:${tried}")
    endif()
    message(FATAL_ERROR "Could not find CLANG_RESOURCE_DIR.${tried}"
      "\n"
      "Please set CLANG_RESOURCE_DIR to the Clang SDK directory containing "
      "\"include/stddef.h\", typically of the form \"<prefix>/lib/clang/${v}\"."
      )
  endif()
  unset(tried)
endif()

install(DIRECTORY ${CLANG_RESOURCE_DIR}/include
  DESTINATION "${CastXML_INSTALL_DATA_DIR}/clang"
  )

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

add_subdirectory(doc)

install(DIRECTORY share/castxml/ DESTINATION "${CastXML_INSTALL_DATA_DIR}")

install(FILES
  "LICENSE"
  "NOTICE"
  DESTINATION "${CastXML_INSTALL_DOC_DIR}"
  )
