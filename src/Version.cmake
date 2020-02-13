# CastXML version number components.
set(CastXML_VERSION_MAJOR 0)
set(CastXML_VERSION_MINOR 2)
set(CastXML_VERSION_PATCH 20200213)
set(CastXML_VERSION_RC 0)
set(CastXML_VERSION_IS_DIRTY 0)

# Start with the full version number used in tags.  It has no dev info.
set(CastXML_VERSION
  "${CastXML_VERSION_MAJOR}.${CastXML_VERSION_MINOR}.${CastXML_VERSION_PATCH}")
if(CastXML_VERSION_RC)
  set(CastXML_VERSION "${CastXML_VERSION}-rc${CastXML_VERSION_RC}")
endif()

if(EXISTS ${CastXML_SOURCE_DIR}/.git)
  find_package(Git QUIET)
  if(GIT_FOUND)
    macro(_git)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} ${ARGN}
        WORKING_DIRECTORY ${CastXML_SOURCE_DIR}
        RESULT_VARIABLE _git_res
        OUTPUT_VARIABLE _git_out OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE _git_err ERROR_STRIP_TRAILING_WHITESPACE
        )
    endmacro()
  endif()
endif()

# Try to identify the current development source version.
if(COMMAND _git)
  # Get the commit checked out in this work tree.
  _git(log -n 1 HEAD "--pretty=format:%h %s" --)
  set(git_info "${_git_out}")
else()
  # Get the commit exported by 'git archive'.
  set(git_info "$Format:%h %s$")
endif()

# Extract commit information if available.
if(git_info MATCHES "^([0-9a-f]+) (.*)$")
  set(git_hash "${CMAKE_MATCH_1}")
  set(git_subject "${CMAKE_MATCH_2}")

  # If this is not the exact commit of a release, add dev info.
  if(NOT "${git_subject}" MATCHES "^[Cc]ast[Xx][Mm][Ll] ${CastXML_VERSION}$")
    set(git_suffix "-g${git_hash}")
    if(COMMAND _git)
      # Use version suffix computed by 'git describe' if this version has been tagged.
      _git(describe --tags --match "v${CastXML_VERSION}" HEAD)
      if(_git_out MATCHES "^v${CastXML_VERSION}(-[0-9]+-g[0-9a-f]+)?$")
        set(git_suffix "${CMAKE_MATCH_1}")
      endif()
    endif()
    set(CastXML_VERSION "${CastXML_VERSION}${git_suffix}")
  endif()

  # If this is a work tree, check whether it is dirty.
  if(COMMAND _git)
    _git(update-index -q --refresh)
    _git(diff-index --name-only HEAD --)
    if(_git_out)
      set(CastXML_VERSION "${CastXML_VERSION}-dirty")
    endif()
  endif()
elseif(NOT "${CastXML_VERSION_PATCH}" VERSION_LESS 20000000)
  # Generic development version.
  set(CastXML_VERSION "${CastXML_VERSION}-git")
endif()
