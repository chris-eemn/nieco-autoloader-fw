# file-format: 1.0.0
#
# Regenerates the git version header (app_version_git.h) that the boot banner prints.
# Run with `cmake -P` from a custom target so it re-queries git on every build, but only
# rewrites the header when the result changes (so the banner source isn't needlessly recompiled).
#
# Expected -D arguments:
#   GIT_EXECUTABLE - path to git (may be empty if git was not found)
#   SRC_DIR        - repo working directory to query
#   OUT_FILE       - header path to (re)generate

if(NOT DEFINED GIT_EXECUTABLE OR GIT_EXECUTABLE STREQUAL "")
  set(GIT_EXECUTABLE "git")
endif()

set(git_describe "unknown")
set(git_commit "unknown")

# `--tags`   consider lightweight tags too, not just annotated
# `--dirty`  append "-dirty" when the working tree has uncommitted changes
# `--always` fall back to a bare short hash if no tag is reachable
execute_process(
  COMMAND "${GIT_EXECUTABLE}" describe --tags --dirty --always
  WORKING_DIRECTORY "${SRC_DIR}"
  OUTPUT_VARIABLE describe_out
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE describe_result
)
if(describe_result EQUAL 0 AND NOT describe_out STREQUAL "")
  set(git_describe "${describe_out}")
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
  WORKING_DIRECTORY "${SRC_DIR}"
  OUTPUT_VARIABLE commit_out
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE commit_result
)
if(commit_result EQUAL 0 AND NOT commit_out STREQUAL "")
  set(git_commit "${commit_out}")
endif()

set(new_content
"/* Auto-generated at build time by cmake/gen_version.cmake -- do not edit, not checked in. */
#ifndef APP_VERSION_GIT_H_
#define APP_VERSION_GIT_H_

#define APP_VERSION_GIT_DESCRIBE \"${git_describe}\"
#define APP_VERSION_GIT_COMMIT   \"${git_commit}\"

#endif /* APP_VERSION_GIT_H_ */
")

set(old_content "")
if(EXISTS "${OUT_FILE}")
  file(READ "${OUT_FILE}" old_content)
endif()

if(NOT old_content STREQUAL new_content)
  file(WRITE "${OUT_FILE}" "${new_content}")
  message(STATUS "git version: ${git_describe} (${git_commit})")
endif()
