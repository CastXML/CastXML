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

execute_process(
  COMMAND ${command}
  OUTPUT_VARIABLE actual_stdout
  ERROR_VARIABLE actual_stderr
  RESULT_VARIABLE actual_result
  )

set(default_result 0)

foreach(o result stdout stderr)
  string(REGEX REPLACE "\n+$" "" actual_${o} "${actual_${o}}")
  string(REGEX REPLACE "\n" "\n actual-${o}> " actual-${o} " actual-${o}> ${actual_${o}}")
  set(actual-${o} "Actual ${o}:\n${actual-${o}}\n")

  set(expect-${o} "")
  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/expect/${test}-${o}.txt)
    file(READ ${CMAKE_CURRENT_LIST_DIR}/expect/${test}-${o}.txt expect_${o})
  elseif(DEFINED default_${o})
    set(expect_${o} "${default_${o}}")
  else()
    unset(expect_${o})
  endif()

  if(DEFINED expect_${o})
    string(REGEX REPLACE "\n+$" "" expect_${o} "${expect_${o}}")
    if(NOT "${actual_${o}}" MATCHES "${expect_${o}}")
      set(msg "${msg}${o} does not match that expected.\n")
      string(REGEX REPLACE "\n" "\n expect-${o}> " expect-${o} " expect-${o}> ${expect_${o}}")
      set(expect-${o} "Expected ${o} to match:\n${expect-${o}}\n")
    endif()
  endif()
endforeach()

if(msg)
  message(SEND_ERROR
    "${msg}"
    "${expect-result}"
    "${expect-stdout}"
    "${expect-stderr}"
    "${actual-result}"
    "${actual-stdout}"
    "${actual-stderr}"
    )
endif()
