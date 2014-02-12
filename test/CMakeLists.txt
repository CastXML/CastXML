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

macro(castxml_test_gccxml test)
  set(command $<TARGET_FILE:castxml>
    --castxml-gccxml --castxml-start start
    -std=c++98
    ${CMAKE_CURRENT_LIST_DIR}/input/${test}.cxx
    )
  add_test(
    NAME gccxml.${test}
    COMMAND ${CMAKE_COMMAND}
    "-Dcommand:STRING=${command}"
    "-Dtest=gccxml.${test}"
    "-Dxml=${test}.xml"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake
    )
endmacro()

castxml_test_cmd(no-arguments)

castxml_test_cmd(cc-missing --castxml-cc-gnu)
castxml_test_cmd(cc-twice --castxml-cc-msvc cl --castxml-cc-gnu gcc)
castxml_test_cmd(cc-unknown --castxml-cc-unknown cc)
castxml_test_cmd(gccxml-and-E --castxml-gccxml -E)
castxml_test_cmd(gccxml-twice --castxml-gccxml --castxml-gccxml)
castxml_test_cmd(o-missing -o)
castxml_test_cmd(start-missing --castxml-start)

castxml_test_gccxml(Class)
castxml_test_gccxml(Class-incomplete)
castxml_test_gccxml(CvQualifiedType)
castxml_test_gccxml(FundamentalType)
castxml_test_gccxml(Namespace)
