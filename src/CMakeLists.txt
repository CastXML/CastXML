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

llvm_map_components_to_libraries(llvm_libs native option bitreader)

add_executable(castxml castxml.cxx)
target_link_libraries(castxml
  cxsys
  ${llvm_libs}
  )
install(TARGETS castxml DESTINATION ${CastXML_INSTALL_RUNTIME_DIR})
