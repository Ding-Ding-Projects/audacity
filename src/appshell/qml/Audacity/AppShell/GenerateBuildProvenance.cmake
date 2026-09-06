if (NOT DEFINED AU_SOURCE_DIR OR NOT DEFINED AU_EXPECTED_SOURCE_REVISION OR NOT DEFINED AU_OUTPUT_HEADER)
    message(FATAL_ERROR "Build provenance generation requires source, expected revision, and output header")
endif()

execute_process(
    COMMAND git -C "${AU_SOURCE_DIR}" rev-parse --verify HEAD
    OUTPUT_VARIABLE AU_ACTUAL_SOURCE_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE AU_ACTUAL_SOURCE_REVISION_RESULT
)
if (NOT AU_ACTUAL_SOURCE_REVISION_RESULT EQUAL 0)
    message(FATAL_ERROR "Cannot resolve the build candidate revision")
endif()

if (NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION)
    message(FATAL_ERROR
        "Build candidate revision changed from ${AU_EXPECTED_SOURCE_REVISION} to ${AU_ACTUAL_SOURCE_REVISION}; reconfigure before building")
endif()

execute_process(COMMAND git -C "${AU_SOURCE_DIR}" diff --quiet --ignore-submodules --
    RESULT_VARIABLE AU_UNSTAGED_DIRT_RESULT)
execute_process(COMMAND git -C "${AU_SOURCE_DIR}" diff --cached --quiet --ignore-submodules --
    RESULT_VARIABLE AU_STAGED_DIRT_RESULT)
execute_process(COMMAND git -C "${AU_SOURCE_DIR}" ls-files --others --exclude-standard
    OUTPUT_VARIABLE AU_UNTRACKED_FILES OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT AU_UNSTAGED_DIRT_RESULT EQUAL 0 OR NOT AU_STAGED_DIRT_RESULT EQUAL 0 OR NOT AU_UNTRACKED_FILES STREQUAL "")
    message(FATAL_ERROR "Build provenance requires a clean configured candidate; commit, remove, or ignore source changes before building")
endif()

execute_process(COMMAND git -C "${AU_SOURCE_DIR}" show -s --format=%cI HEAD
    OUTPUT_VARIABLE AU_BUILD_TIMESTAMP_UTC OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE AU_BUILD_TIMESTAMP_RESULT)
if (NOT AU_BUILD_TIMESTAMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Cannot resolve the configured candidate timestamp")
endif()
get_filename_component(AU_OUTPUT_DIRECTORY "${AU_OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${AU_OUTPUT_DIRECTORY}")
string(CONCAT AU_HEADER_CONTENT
    "#pragma once\n"
    "#define AU_BUILD_TIMESTAMP_UTC \"${AU_BUILD_TIMESTAMP_UTC}\"\n"
    "#define AU_BUILD_SOURCE_REVISION \"${AU_ACTUAL_SOURCE_REVISION}\"\n")
set(AU_EXISTING_HEADER "")
if (EXISTS "${AU_OUTPUT_HEADER}")
    file(READ "${AU_OUTPUT_HEADER}" AU_EXISTING_HEADER)
endif()
if (NOT AU_EXISTING_HEADER STREQUAL AU_HEADER_CONTENT)
    file(WRITE "${AU_OUTPUT_HEADER}.tmp" "${AU_HEADER_CONTENT}")
    file(RENAME "${AU_OUTPUT_HEADER}.tmp" "${AU_OUTPUT_HEADER}")
endif()
