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

find_package(LibXml2 QUIET)

macro(castxml_test_cmd test)
  set(command $<TARGET_FILE:castxml> ${ARGN})
  add_test(
    NAME cmd.${test}
    COMMAND ${CMAKE_COMMAND}
    "-Dcommand:STRING=${command}"
    "-Dtest=cmd.${test}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake
    )
endmacro()

macro(castxml_test_gccxml_common prefix ext std test)
  set(command $<TARGET_FILE:castxml>
    --castxml-gccxml --castxml-start start
    ${std}
    ${CMAKE_CURRENT_LIST_DIR}/input/${test}.${ext}
    )
  add_test(
    NAME ${prefix}.${test}
    COMMAND ${CMAKE_COMMAND}
    "-Dcommand:STRING=${command}"
    "-Dtest=${prefix}.${test}"
    "-Dxml=${test}.xml"
    "-Dxmllint=${LIBXML2_XMLLINT_EXECUTABLE}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake
    )
endmacro()

macro(castxml_test_gccxml test)
  castxml_test_gccxml_common(gccxml cxx -std=c++98 ${test})
endmacro()

macro(castxml_test_gccxml_c test)
  castxml_test_gccxml_common(gccxml.c c -std=c89 ${test})
endmacro()

set(empty_c ${CMAKE_CURRENT_LIST_DIR}/input/empty.c)
set(empty_cxx ${CMAKE_CURRENT_LIST_DIR}/input/empty.cxx)
set(empty_m ${CMAKE_CURRENT_LIST_DIR}/input/empty.m)
set(empty_mm ${CMAKE_CURRENT_LIST_DIR}/input/empty.mm)

castxml_test_cmd(no-arguments)

castxml_test_cmd(cc-missing --castxml-cc-gnu)
castxml_test_cmd(cc-twice --castxml-cc-msvc cl --castxml-cc-gnu gcc)
castxml_test_cmd(cc-unknown --castxml-cc-unknown cc)
castxml_test_cmd(gccxml-and-E --castxml-gccxml -E)
castxml_test_cmd(gccxml-twice --castxml-gccxml --castxml-gccxml)
castxml_test_cmd(gccxml-and-c99 --castxml-gccxml -std=c99 ${empty_c})
castxml_test_cmd(gccxml-and-c11 --castxml-gccxml -std=c11 ${empty_c})
castxml_test_cmd(gccxml-and-c++11 --castxml-gccxml -std=c++11 ${empty_cxx})
castxml_test_cmd(gccxml-and-c++1y --castxml-gccxml -std=c++1y ${empty_cxx})
castxml_test_cmd(gccxml-and-objc1 --castxml-gccxml ${empty_m})
castxml_test_cmd(gccxml-and-objc2 --castxml-gccxml ${empty_mm})
castxml_test_cmd(o-missing -o)
castxml_test_cmd(start-missing --castxml-start)

castxml_test_gccxml(ArrayType)
castxml_test_gccxml(ArrayType-incomplete)
castxml_test_gccxml(Class)
castxml_test_gccxml(Class-abstract)
castxml_test_gccxml(Class-bases)
castxml_test_gccxml(Class-friends)
castxml_test_gccxml(Class-incomplete)
castxml_test_gccxml(Class-member-template)
castxml_test_gccxml(Class-template)
castxml_test_gccxml(Class-template-member-template)
castxml_test_gccxml(CvQualifiedType)
castxml_test_gccxml(Enumeration)
castxml_test_gccxml(Field)
castxml_test_gccxml(Function)
castxml_test_gccxml(Function-template)
castxml_test_gccxml(Function-throw)
castxml_test_gccxml(Function-variadic)
castxml_test_gccxml(FunctionType)
castxml_test_gccxml(FunctionType-variadic)
castxml_test_gccxml(FundamentalType)
castxml_test_gccxml(FundamentalTypes)
castxml_test_gccxml(Method)
castxml_test_gccxml(MethodType)
castxml_test_gccxml(MethodType-cv)
castxml_test_gccxml(Namespace)
castxml_test_gccxml(OffsetType)
castxml_test_gccxml(OffsetType-cv)
castxml_test_gccxml(OperatorFunction)
castxml_test_gccxml(OperatorMethod)
castxml_test_gccxml(PointerType)
castxml_test_gccxml(ReferenceType)
castxml_test_gccxml(Typedef-paren)
castxml_test_gccxml(Typedef-to-Class-template)
castxml_test_gccxml(Typedef-to-Enumeration)
castxml_test_gccxml(Variable)
castxml_test_gccxml(Variable-in-Class)
castxml_test_gccxml(Variable-init)

castxml_test_gccxml(qualified-type-name)
castxml_test_gccxml(using-declaration-class)
castxml_test_gccxml(using-declaration-ns)
castxml_test_gccxml(using-declaration-start)
castxml_test_gccxml(using-directive-ns)
castxml_test_gccxml(using-directive-start)

castxml_test_gccxml_c(FundamentalType)
castxml_test_gccxml_c(Typedef-called-class)

castxml_test_gccxml_c(invalid)
