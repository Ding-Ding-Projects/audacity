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

string(TIMESTAMP AU_BUILD_TIMESTAMP_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)
get_filename_component(AU_OUTPUT_DIRECTORY "${AU_OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${AU_OUTPUT_DIRECTORY}")
file(WRITE "${AU_OUTPUT_HEADER}.tmp"
    "#pragma once\n"
    "#define AU_BUILD_TIMESTAMP_UTC \"${AU_BUILD_TIMESTAMP_UTC}\"\n"
    "#define AU_BUILD_SOURCE_REVISION \"${AU_ACTUAL_SOURCE_REVISION}\"\n")
file(RENAME "${AU_OUTPUT_HEADER}.tmp" "${AU_OUTPUT_HEADER}")
