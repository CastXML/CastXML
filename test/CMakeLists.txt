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
