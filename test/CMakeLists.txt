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

# Keep test binaries away from main binaries.
unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)

find_package(LibXml2 QUIET)

macro(castxml_test_cmd test)
  set(command $<TARGET_FILE:castxml> ${ARGN})
  add_test(
    NAME cmd.${test}
    COMMAND ${CMAKE_COMMAND}
    "-Dcommand:STRING=${command}"
    "-Dexpect=cmd.${test}"
    ${castxml_test_cmd_extra_arguments}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake
    )
endmacro()

macro(castxml_test_gccxml_common prefix ext std test)
  if(castxml_test_gccxml_custom_start)
    set(_castxml_start ${castxml_test_gccxml_custom_start})
  else()
    set(_castxml_start --castxml-start start)
  endif()
  if(castxml_test_gccxml_custom_input)
    set(_castxml_input ${castxml_test_gccxml_custom_input})
  else()
    set(_castxml_input ${test})
  endif()
  set(command $<TARGET_FILE:castxml>
    --castxml-gccxml
    ${_castxml_start}
    -std=${std}
    ${CMAKE_CURRENT_LIST_DIR}/input/${_castxml_input}.${ext}
    -o ${prefix}.${std}.${test}.xml
    ${castxml_test_gccxml_extra_arguments}
    )
  add_test(
    NAME ${prefix}.${std}.${test}
    COMMAND ${CMAKE_COMMAND}
    "-Dcommand:STRING=${command}"
    "-Dexpect=${prefix}.${std}.${test};${prefix}.any.${test}"
    "-Dxml=${prefix}.${std}.${test}.xml"
    "-Dxmllint=${LIBXML2_XMLLINT_EXECUTABLE}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake
    )
endmacro()

macro(castxml_test_gccxml_c89 test)
  castxml_test_gccxml_common(gccxml c c89 ${test})
endmacro()

macro(castxml_test_gccxml_cxx98 test)
  castxml_test_gccxml_common(gccxml cxx c++98 ${test})
endmacro()

macro(castxml_test_gccxml_cxx11 test)
  castxml_test_gccxml_common(gccxml cxx c++11 ${test})
endmacro()

macro(castxml_test_gccxml_broken_cxx98 test)
  castxml_test_gccxml_common(gccxml.broken cxx c++98 ${test})
endmacro()

macro(castxml_test_gccxml_broken_cxx11 test)
  castxml_test_gccxml_common(gccxml.broken cxx c++11 ${test})
endmacro()

macro(castxml_test_gccxml_c test)
  castxml_test_gccxml_c89(${test})
endmacro()

macro(castxml_test_gccxml test)
  castxml_test_gccxml_cxx98(${test})
  castxml_test_gccxml_cxx11(${test})
endmacro()

macro(castxml_test_gccxml_broken test)
  castxml_test_gccxml_broken_cxx98(${test})
  castxml_test_gccxml_broken_cxx11(${test})
endmacro()

set(input ${CMAKE_CURRENT_LIST_DIR}/input)
set(empty_c ${input}/empty.c)
set(empty_cxx ${input}/empty.cxx)
set(empty_m ${input}/empty.m)
set(empty_mm ${input}/empty.mm)

castxml_test_cmd(help1 -help)
castxml_test_cmd(help2 --help)
castxml_test_cmd(no-arguments)
castxml_test_cmd(version --version)

castxml_test_cmd(cc-missing --castxml-cc-gnu)
castxml_test_cmd(cc-option --castxml-cc-gnu -)
castxml_test_cmd(cc-paren-castxml --castxml-cc-gnu "(" --castxml-cc-msvc ")")
castxml_test_cmd(cc-paren-nested --castxml-cc-gnu "(" "(" ")" ")")
castxml_test_cmd(cc-paren-unbalanced --castxml-cc-gnu "(")
castxml_test_cmd(cc-twice --castxml-cc-msvc cl --castxml-cc-gnu gcc)
castxml_test_cmd(cc-unknown --castxml-cc-unknown cc)
castxml_test_cmd(gccxml-and-E --castxml-gccxml -E)
castxml_test_cmd(gccxml-twice --castxml-gccxml --castxml-gccxml)
castxml_test_cmd(gccxml-and-c99 --castxml-gccxml -std=c99 ${empty_c})
castxml_test_cmd(gccxml-and-c11 --castxml-gccxml -std=c11 ${empty_c})
castxml_test_cmd(gccxml-and-c++11 --castxml-gccxml -std=c++11 ${empty_cxx})
castxml_test_cmd(gccxml-and-c++14 --castxml-gccxml -std=c++14 ${empty_cxx})
castxml_test_cmd(gccxml-and-objc1 --castxml-gccxml ${empty_m})
castxml_test_cmd(gccxml-and-objc2 --castxml-gccxml ${empty_mm})
castxml_test_cmd(gccxml-empty-c++98 --castxml-gccxml -std=c++98 ${empty_cxx})
castxml_test_cmd(gccxml-empty-c++98-E --castxml-gccxml -std=c++98 ${empty_cxx} -E)
castxml_test_cmd(gccxml-empty-c++98-c --castxml-gccxml -std=c++98 ${empty_cxx} -c)
castxml_test_cmd(o-missing -o)
castxml_test_cmd(start-missing --castxml-start)
castxml_test_cmd(rsp-empty @${input}/empty.rsp)
castxml_test_cmd(rsp-missing @${input}/does-not-exist.rsp)
castxml_test_cmd(rsp-o-missing @${input}/o-missing.rsp)

# Test --castxml-cc-gnu detection.
add_executable(cc-gnu cc-gnu.c)
set_property(SOURCE cc-gnu.c APPEND PROPERTY COMPILE_DEFINITIONS
  "TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
castxml_test_cmd(cc-gnu-c-E --castxml-cc-gnu $<TARGET_FILE:cc-gnu> ${empty_c} -E -dM)
castxml_test_cmd(cc-gnu-c-cmd --castxml-cc-gnu $<TARGET_FILE:cc-gnu> ${empty_c} "-###")
castxml_test_cmd(cc-gnu-cxx-E --castxml-cc-gnu $<TARGET_FILE:cc-gnu> ${empty_cxx} -E -dM)
castxml_test_cmd(cc-gnu-cxx-cmd --castxml-cc-gnu $<TARGET_FILE:cc-gnu> ${empty_cxx} "-###")
castxml_test_cmd(cc-gnu-tgt-amd64 --castxml-cc-gnu "(" $<TARGET_FILE:cc-gnu> --cc-define=__amd64__ ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-gnu-tgt-explicit --castxml-cc-gnu "(" $<TARGET_FILE:cc-gnu> ")" -target explicit-target-triple ${empty_cxx} "-###")
castxml_test_cmd(cc-gnu-tgt-i386 --castxml-cc-gnu "(" $<TARGET_FILE:cc-gnu> --cc-define=__i386__ ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-gnu-tgt-mingw --castxml-cc-gnu "(" $<TARGET_FILE:cc-gnu> --cc-define=_WIN32 --cc-define=__MINGW32__ ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-gnu-tgt-win --castxml-cc-gnu "(" $<TARGET_FILE:cc-gnu> --cc-define=_WIN32 ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-gnu-tgt-x86_64 --castxml-cc-gnu "(" $<TARGET_FILE:cc-gnu> --cc-define=__x86_64__ ")" ${empty_cxx} "-###")

# Test --castxml-cc-msvc detection.
add_executable(cc-msvc cc-msvc.c)
set(castxml_test_cmd_extra_arguments "-Dprologue=${CMAKE_CURRENT_SOURCE_DIR}/cc-msvc.cmake")
castxml_test_cmd(cc-msvc-c-E --castxml-cc-msvc $<TARGET_FILE:cc-msvc> ${empty_c} -E -dM)
castxml_test_cmd(cc-msvc-c-cmd --castxml-cc-msvc $<TARGET_FILE:cc-msvc> ${empty_c} "-###")
castxml_test_cmd(cc-msvc-cxx-E --castxml-cc-msvc $<TARGET_FILE:cc-msvc> ${empty_cxx} -E -dM)
castxml_test_cmd(cc-msvc-cxx-cmd --castxml-cc-msvc $<TARGET_FILE:cc-msvc> ${empty_cxx} "-###")
castxml_test_cmd(cc-msvc-tgt-amd64 --castxml-cc-msvc "(" $<TARGET_FILE:cc-msvc> --cc-define=_M_AMD64 ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-msvc-tgt-explicit --castxml-cc-msvc "(" $<TARGET_FILE:cc-msvc> ")" -target explicit-target-triple ${empty_cxx} "-###")
castxml_test_cmd(cc-msvc-tgt-i386 --castxml-cc-msvc "(" $<TARGET_FILE:cc-msvc> --cc-define=_M_IX86 ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-msvc-tgt-win --castxml-cc-msvc "(" $<TARGET_FILE:cc-msvc> --cc-define=_WIN32 ")" ${empty_cxx} "-###")
castxml_test_cmd(cc-msvc-tgt-x86_64 --castxml-cc-msvc "(" $<TARGET_FILE:cc-msvc> --cc-define=_M_X64 ")" ${empty_cxx} "-###")
unset(castxml_test_cmd_extra_arguments)

castxml_test_gccxml(ArrayType)
castxml_test_gccxml(ArrayType-incomplete)
castxml_test_gccxml(Class)
castxml_test_gccxml(Class-abstract)
castxml_test_gccxml(Class-base-typedef)
castxml_test_gccxml(Class-bases)
castxml_test_gccxml(Class-forward)
castxml_test_gccxml(Class-friends)
castxml_test_gccxml(Class-implicit-member-access)
castxml_test_gccxml(Class-implicit-member-access-mutable)
castxml_test_gccxml(Class-implicit-member-array)
castxml_test_gccxml(Class-implicit-member-bad-base)
castxml_test_gccxml(Class-implicit-member-const)
castxml_test_gccxml(Class-implicit-member-reference)
castxml_test_gccxml(Class-implicit-members)
castxml_test_gccxml(Class-incomplete)
castxml_test_gccxml(Class-incomplete-twice)
castxml_test_gccxml(Class-member-template)
castxml_test_gccxml(Class-partial-template-member-Typedef)
castxml_test_gccxml(Class-template)
castxml_test_gccxml(Class-template-Method-Argument-const)
castxml_test_gccxml(Class-template-Method-Argument-default)
castxml_test_gccxml(Class-template-Method-return-const)
castxml_test_gccxml(Class-template-bases)
castxml_test_gccxml(Class-template-constructor-template)
castxml_test_gccxml(Class-template-friends)
castxml_test_gccxml(Class-template-member-Typedef)
castxml_test_gccxml(Class-template-member-Typedef-const)
castxml_test_gccxml(Class-template-member-template)
castxml_test_gccxml(CvQualifiedType)
castxml_test_gccxml(Enumeration)
castxml_test_gccxml(Enumeration-anonymous)
castxml_test_gccxml(Field)
castxml_test_gccxml(Function)
castxml_test_gccxml(Function-Argument-decay)
castxml_test_gccxml(Function-Argument-default)
castxml_test_gccxml(Function-rvalue-reference)
castxml_test_gccxml(Function-template)
castxml_test_gccxml(Function-throw)
castxml_test_gccxml(Function-variadic)
castxml_test_gccxml(FunctionType)
castxml_test_gccxml(FunctionType-variadic)
castxml_test_gccxml(FundamentalType)
castxml_test_gccxml(FundamentalTypes)
castxml_test_gccxml(Method)
castxml_test_gccxml(Method-rvalue-reference)
castxml_test_gccxml(MethodType)
castxml_test_gccxml(MethodType-cv)
castxml_test_gccxml(Namespace)
castxml_test_gccxml(Namespace-anonymous)
castxml_test_gccxml(Namespace-Class-members)
castxml_test_gccxml(Namespace-Class-partial-template-members)
castxml_test_gccxml(Namespace-Class-template-members)
castxml_test_gccxml(Namespace-Function-template-members)
castxml_test_gccxml(Namespace-empty)
castxml_test_gccxml(Namespace-extern-C-members)
castxml_test_gccxml(Namespace-inline)
castxml_test_gccxml(Namespace-inline-start)
castxml_test_gccxml(Namespace-inline-template)
castxml_test_gccxml(Namespace-nested)
castxml_test_gccxml(Namespace-repeat)
castxml_test_gccxml(Namespace-repeat-start)
castxml_test_gccxml(OffsetType)
castxml_test_gccxml(OffsetType-cv)
castxml_test_gccxml(OperatorFunction)
castxml_test_gccxml(OperatorMethod)
castxml_test_gccxml(PointerType)
castxml_test_gccxml(ReferenceType)
castxml_test_gccxml(RValueReferenceType)
castxml_test_gccxml(Typedef-paren)
castxml_test_gccxml(Typedef-to-Class-template)
castxml_test_gccxml(Typedef-to-Enumeration)
castxml_test_gccxml(Typedef-to-Enumeration-anonymous)
castxml_test_gccxml(Typedef-to-FundamentalType-mode)
castxml_test_gccxml(Typedef-to-extern-C-FundamentalType-mode)
castxml_test_gccxml(Variable)
castxml_test_gccxml(Variable-in-Class)
castxml_test_gccxml(Variable-init)

castxml_test_gccxml(qualified-type-name)
castxml_test_gccxml(using-declaration-class)
castxml_test_gccxml(using-declaration-ns)
castxml_test_gccxml(using-declaration-start)
castxml_test_gccxml(using-directive-ns)
castxml_test_gccxml(using-directive-start)

if(";${LLVM_TARGETS_TO_BUILD};" MATCHES ";X86;")
  set(castxml_test_gccxml_extra_arguments -target i386-pc-windows-msvc)
  castxml_test_gccxml(Function-calling-convention-ms)
  castxml_test_gccxml(implicit-decl-ms)
  castxml_test_gccxml(inline-asm-ms)
  unset(castxml_test_gccxml_extra_arguments)
endif()

castxml_test_gccxml_c(FunctionNoProto)
castxml_test_gccxml_c(FundamentalType)
castxml_test_gccxml_c(Typedef-called-class)

castxml_test_gccxml_c(invalid)

castxml_test_gccxml_broken(ReferenceType-to-Class-template)

#-----------------------------------------------------------------------------
# Find a real GNU compiler to test with --castxml-cc-gnu.

set(_gnu_C gcc)
set(_gnu_CXX g++)

foreach(lang C CXX)
  find_program(TEST_GNU_${lang} NAMES ${_gnu_${lang}} DOC "Path to GNU ${_gnu_${lang}} compiler for testing")
  # Exclude the Apple LLVM tool that is not really a GNU compiler.
  if(TEST_GNU_${lang})
    execute_process(
      COMMAND ${TEST_GNU_${lang}} --version
      OUTPUT_VARIABLE version
      ERROR_VARIABLE version
      RESULT_VARIABLE failed
      )
    if(failed OR version MATCHES "Apple LLVM")
      set(TEST_GNU_${lang} TEST_GNU_${lang}-NOTFOUND)
    endif()
  endif()
  if(TEST_GNU_${lang})
    set(castxml_test_gccxml_extra_arguments --castxml-cc-gnu ${TEST_GNU_C})
    message(STATUS "Found GNU ${lang} compiler '${TEST_GNU_C}' to drive tests")
  endif()
endforeach()

if(TEST_GNU_C)
  set(castxml_test_gccxml_extra_arguments --castxml-cc-gnu ${TEST_GNU_C})
  castxml_test_gccxml_c(GNU-float128)
  unset(castxml_test_gccxml_extra_arguments)
endif()

if(TEST_GNU_CXX)
  set(castxml_test_gccxml_extra_arguments --castxml-cc-gnu ${TEST_GNU_CXX})
  castxml_test_gccxml(GNU-float128)
  unset(castxml_test_gccxml_extra_arguments)
endif()
