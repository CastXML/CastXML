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

if(xml)
  file(REMOVE "${xml}")
endif()

if(prologue)
  include(${prologue})
endif()

execute_process(
  COMMAND ${command}
  OUTPUT_VARIABLE actual_stdout
  ERROR_VARIABLE actual_stderr
  RESULT_VARIABLE actual_result
  )

if(xml)
  set(maybe_xml xml)
  if(EXISTS "${xml}")
    file(READ "${xml}" actual_xml)
  else()
    set(actual_xml "(missing)")
  endif()
else()
  set(maybe_xml)
endif()

set(default_result 0)
set(default_stdout "^$")
set(default_stderr "^$")

foreach(o result stdout stderr ${maybe_xml})
  string(REGEX REPLACE "\n+$" "" actual_${o} "${actual_${o}}")
  string(REGEX REPLACE "\n" "\n actual-${o}> " actual-${o} " actual-${o}> ${actual_${o}}")
  set(actual-${o} "Actual ${o}:\n${actual-${o}}\n")

  set(expect-${o} "")
  unset(expect_${o})
  foreach(e ${expect})
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/expect/${e}.${o}.txt)
      file(READ ${CMAKE_CURRENT_LIST_DIR}/expect/${e}.${o}.txt expect_${o})
      break()
    endif()
  endforeach()
  if(NOT DEFINED expect_${o} AND DEFINED default_${o})
    set(expect_${o} "${default_${o}}")
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
    "${expect-xml}"
    "${actual-result}"
    "${actual-stdout}"
    "${actual-stderr}"
    "${actual-xml}"
    )
endif()

if(xmllint AND xml AND EXISTS "${xml}")
  execute_process(
    COMMAND ${xmllint} --noout --nonet "${xml}"
    OUTPUT_VARIABLE xmllint_stdout
    ERROR_VARIABLE xmllint_stderr
    RESULT_VARIABLE xmllint_result
    )
  if(xmllint_result)
    foreach(o result stdout stderr)
      string(REGEX REPLACE "\n+$" "" xmllint_${o} "${xmllint_${o}}")
      string(REGEX REPLACE "\n" "\n xmllint-${o}> " xmllint-${o} " xmllint-${o}> ${xmllint_${o}}")
      set(xmllint-${o} "xmllint ${o}:\n${xmllint-${o}}\n")
    endforeach()
    message(SEND_ERROR
      "xmllint check failed:\n"
      "${msg}"
      "${xmllint-result}"
      "${xmllint-stdout}"
      "${xmllint-stderr}"
      )
  endif()
endif()
