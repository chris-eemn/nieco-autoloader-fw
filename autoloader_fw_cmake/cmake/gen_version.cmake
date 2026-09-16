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

# Numeric major/minor/build, pulled out of the same describe string so a version comparison
# never needs to parse text at runtime. The USB firmware-update loader compares these against
# the `version_major`/`version_minor`/`version_build` fields of an update file's
# ede_update_file_header_t (usb_loader/update_image.h), where each is a uint8_t -- hence the
# 0..255 range check below.
#
# Only the tag part is numeric: the `-<n>-g<hash>`/`-dirty` suffixes git describe appends to an
# untagged or dirty build are deliberately discarded, so a development build made 10 commits
# past v0.0.1 compares equal to a v0_0_1 update file. APP_VERSION_GIT_DESCRIBE remains the
# string that identifies the exact build.
#
# APP_VERSION_NUMERIC_VALID is 0 when no vX.Y.Z tag is reachable (git missing, shallow clone, or
# a repo that has never been tagged) -- the loader then reports the running version as unknown
# rather than silently comparing against 0.0.0.
set(app_version_major 0)
set(app_version_minor 0)
set(app_version_build 0)
set(app_version_numeric_valid 0)

if(git_describe MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  if(CMAKE_MATCH_1 LESS_EQUAL 255 AND CMAKE_MATCH_2 LESS_EQUAL 255 AND CMAKE_MATCH_3 LESS_EQUAL 255)
    set(app_version_major ${CMAKE_MATCH_1})
    set(app_version_minor ${CMAKE_MATCH_2})
    set(app_version_build ${CMAKE_MATCH_3})
    set(app_version_numeric_valid 1)
  else()
    message(WARNING "git tag '${git_describe}' has a version field above 255; it does not fit the "
                    "update-file header's uint8_t version fields, so the running version is "
                    "reported as unknown to the USB update loader.")
  endif()
endif()

set(new_content
"/* Auto-generated at build time by cmake/gen_version.cmake -- do not edit, not checked in. */
#ifndef APP_VERSION_GIT_H_
#define APP_VERSION_GIT_H_

#define APP_VERSION_GIT_DESCRIBE \"${git_describe}\"
#define APP_VERSION_GIT_COMMIT   \"${git_commit}\"

/* Numeric form of the vX.Y.Z tag in APP_VERSION_GIT_DESCRIBE, for comparison against an update
 * file's header. All three are 0 and APP_VERSION_NUMERIC_VALID is 0 when no such tag exists. */
#define APP_VERSION_MAJOR ${app_version_major}
#define APP_VERSION_MINOR ${app_version_minor}
#define APP_VERSION_BUILD ${app_version_build}
#define APP_VERSION_NUMERIC_VALID ${app_version_numeric_valid}

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
