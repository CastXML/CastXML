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
    # Filter out arch-specific attributes.
    string(REGEX REPLACE "(<(Constructor|Destructor|Method|OperatorMethod|Converter)[^/>]*) attributes=\"__thiscall__\"(/?>)" "\\1\\3" actual_xml "${actual_xml}")
    string(REGEX REPLACE "(<(Constructor|Destructor|Method|OperatorMethod|Converter)[^/>]*) attributes=\"__thiscall__ ([^/>]+)\"(/?>)" "\\1 attributes=\"\\3\"\\4" actual_xml "${actual_xml}")
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
  string(REGEX REPLACE "(^|\n)warning: unknown platform, assuming -mfloat-abi=soft\n" "\\1" actual_${o} "${actual_${o}}")
  string(REGEX REPLACE "\n+$" "" actual_${o} "${actual_${o}}")
  string(REGEX REPLACE "\n" "\n actual-${o}> " actual-${o} " actual-${o}> ${actual_${o}}")
  set(actual-${o} "Actual ${o}:\n${actual-${o}}\n")

  set(expect-${o} "")
  unset(expect_${o})
  foreach(e ${expect})
    if(IS_ABSOLUTE "${e}")
      set(f "${e}.${o}.txt")
    else()
      set(f ${CMAKE_CURRENT_LIST_DIR}/expect/${e}.${o}.txt)
    endif()
    if(EXISTS "${f}")
      file(READ "${f}" expect_${o})
      set(expect_${o}_file "${f}")
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

if(maybe_xml AND actual_xml MATCHES "__castxml")
  set(msg "xml contains disallowed text '__castxml'\n")
endif()

if(msg)
  if("$ENV{TEST_UPDATE}" AND expect_xml_file AND EXISTS "${xml}")
    set(update_xml "${actual_xml}")
    string(REGEX REPLACE "^<\\?xml version=\"1.0\"\\?>" "^<\\\\?xml version=\"1.0\"\\\\?>" update_xml "${update_xml}")
    string(REGEX REPLACE "([][(*)])" "\\\\\\1" update_xml "${update_xml}")
    string(REGEX REPLACE "<CastXML[^>]*>" "<CastXML[^>]*>" update_xml "${update_xml}")
    string(REGEX REPLACE "<GCC_XML[^>]*>" "<GCC_XML[^>]*>" update_xml "${update_xml}")
    string(REGEX REPLACE "mangled=\"[^\"]*\"" "mangled=\"[^\"]+\"" update_xml "${update_xml}")
    string(REGEX REPLACE "artificial=\"1\"( throw=\"\")?" "artificial=\"1\"( throw=\"\")?" update_xml "${update_xml}")
    string(REGEX REPLACE "size=\"[0-9]+\" align=\"[0-9]+\"" "size=\"[0-9]+\" align=\"[0-9]+\"" update_xml "${update_xml}")
    string(REGEX REPLACE "<File id=\"(f[0-9]+)\" name=\"[^\"]*/test/input/([^\"]*)\"/>" "<File id=\"\\1\" name=\".*/test/input/\\2\"/>" update_xml "${update_xml}")
    if(update_xml MATCHES "<Base.*<Base") # multiple inheritance has ABI-specific offsets
      string(REGEX REPLACE "<Base type=\"([^\"]*)\" access=\"([^\"]*)\" virtual=\"0\" offset=\"[0-9]+\"/>" "<Base type=\"\\1\" access=\"\\2\" virtual=\"0\" offset=\"[0-9]+\"/>" update_xml "${update_xml}")
    endif()
    string(REGEX REPLACE "</GCC_XML>$" "</GCC_XML>$" update_xml "${update_xml}")
    string(REGEX REPLACE "</CastXML>$" "</CastXML>$" update_xml "${update_xml}")
    file(WRITE "${expect_xml_file}" "${update_xml}\n")
  endif()

  string(REPLACE ";" "\" \"" command_string "\"${command}\"")
  set(msg "${msg}Command was:\n command> ${command_string}\n")
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
