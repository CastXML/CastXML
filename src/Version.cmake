set(CastXML_VERSION "0.1")

if(EXISTS ${CastXML_SOURCE_DIR}/.git)
  find_package(Git)
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

if(EXISTS ${CastXML_SOURCE_DIR}/VERSION)
  # Read version provided in file with source tarball.
  file(STRINGS ${CastXML_SOURCE_DIR}/VERSION CastXML_VERSION
       LIMIT_COUNT 1 LIMIT_INPUT 1024)
elseif(COMMAND _git)
  # Compute version relative to annotated version tags, if any.
  _git(describe --match "v[0-9]*" --dirty)
  if(_git_out MATCHES "^v([0-9].*)$")
    # Use version computed by 'git describe'.
    set(CastXML_VERSION "${CMAKE_MATCH_1}")
  else()
    # Compute version currently checked out, possibly dirty.
    _git(rev-parse --verify -q --short=7 HEAD)
    if(_git_out MATCHES "^([0-9a-f]+)$")
      set(CastXML_VERSION "${CastXML_VERSION}-g${CMAKE_MATCH_1}")
      _git(update-index -q --refresh)
      _git(diff-index --name-only HEAD --)
      if(_git_out)
        set(CastXML_VERSION "${CastXML_VERSION}-dirty")
      endif()
    else()
      set(CastXML_VERSION "${CastXML_VERSION}-git")
    endif()
  endif()
elseif("$Format:%h$" MATCHES "^([0-9a-f]+)$")
  # Use version exported by 'git archive'.
  set(CastXML_VERSION "${CastXML_VERSION}-g${CMAKE_MATCH_1}")
else()
  # Generic development version.
  set(CastXML_VERSION "${CastXML_VERSION}-git")
endif()
