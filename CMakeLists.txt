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
  set(CastXML_INSTALL_DOC_DIR doc/castxml)
endif()
if(NOT CastXML_INSTALL_MAN_DIR)
  set(CastXML_INSTALL_MAN_DIR man)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=c++11")
endif()

set(KWSYS_NAMESPACE cxsys)
set(KWSYS_USE_Process 1)
set(KWSYS_USE_RegularExpression 1)
set(KWSYS_USE_SystemTools 1)
set(KWSYS_HEADER_ROOT ${CastXML_BINARY_DIR}/src)
add_subdirectory(src/kwsys)
include_directories(${KWSYS_HEADER_ROOT})

find_package(LLVM REQUIRED)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
if(NOT DEFINED LLVM_VERSION_PATCH)
  set(LLVM_VERSION_PATCH 0)
endif()

set(CLANG_RESOURCE_DIR "" CACHE PATH "Clang resource directory")
if(NOT CLANG_RESOURCE_DIR)
  set(v ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH})
  foreach(d ${LLVM_LIBRARY_DIRS})
    if(IS_DIRECTORY "${d}/clang/${v}/include")
      set(CLANG_RESOURCE_DIR ${d}/clang/${v})
      break()
    endif()
  endforeach()
endif()

if(CLANG_RESOURCE_DIR)
  install(DIRECTORY ${CLANG_RESOURCE_DIR}/include
    DESTINATION "${CastXML_INSTALL_DATA_DIR}/clang"
    )
endif()

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
