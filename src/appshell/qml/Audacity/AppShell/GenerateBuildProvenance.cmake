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

execute_process(COMMAND git -C "${AU_SOURCE_DIR}" diff --quiet --ignore-submodules=dirty --
    RESULT_VARIABLE AU_UNSTAGED_DIRT_RESULT)
execute_process(COMMAND git -C "${AU_SOURCE_DIR}" diff --cached --quiet --ignore-submodules=dirty --
    RESULT_VARIABLE AU_STAGED_DIRT_RESULT)
execute_process(COMMAND git -C "${AU_SOURCE_DIR}" ls-files --others --exclude-standard
    OUTPUT_VARIABLE AU_UNTRACKED_FILES OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE AU_UNTRACKED_FILES_RESULT)
if (NOT AU_UNSTAGED_DIRT_RESULT EQUAL 0 OR NOT AU_STAGED_DIRT_RESULT EQUAL 0 OR NOT AU_UNTRACKED_FILES_RESULT EQUAL 0 OR NOT AU_UNTRACKED_FILES STREQUAL "")
    message(FATAL_ERROR "Build provenance requires a clean configured candidate; commit, remove, or ignore source changes before building")
endif()

execute_process(COMMAND git -C "${AU_SOURCE_DIR}" rev-parse "HEAD^{tree}"
    OUTPUT_VARIABLE AU_SOURCE_TREE OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE AU_SOURCE_TREE_RESULT)
if(NOT AU_SOURCE_TREE_RESULT EQUAL 0)
    message(FATAL_ERROR "Cannot resolve the configured source tree")
endif()

# Version fields are build metadata, never inferred from the runtime clock.
# Missing metadata remains empty and is rendered unavailable by the consumer.
function(au_json_string output value)
    string(REPLACE "\\" "\\\\" escaped "${value}")
    string(REPLACE "\"" "\\\"" escaped "${escaped}")
    string(REPLACE "\n" "\\n" escaped "${escaped}")
    string(REPLACE "\r" "\\r" escaped "${escaped}")
    string(REPLACE "\t" "\\t" escaped "${escaped}")
    set(${output} "\"${escaped}\"" PARENT_SCOPE)
endfunction()

set(AU_FACTS "{}")
string(JSON AU_FACTS SET "${AU_FACTS}" schemaVersion 1)
foreach(pair IN ITEMS "version|AU_BUILD_VERSION" "versionLabel|AU_BUILD_VERSION_LABEL"
    "buildNumber|AU_BUILD_NUMBER" "buildId|AU_BUILD_ID" "sourceRevision|AU_EXPECTED_SOURCE_REVISION" "sourceTree|AU_SOURCE_TREE")
    string(REPLACE "|" ";" fields "${pair}")
    list(GET fields 0 key)
    list(GET fields 1 variable)
    au_json_string(value "${${variable}}")
    string(JSON AU_FACTS SET "${AU_FACTS}" "${key}" "${value}")
endforeach()
string(JSON AU_FACTS SET "${AU_FACTS}" timestampKind "\"build-start\"")
string(SHA256 AU_FACTS_HASH "${AU_FACTS}")
get_filename_component(AU_OUTPUT_DIRECTORY "${AU_OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${AU_OUTPUT_DIRECTORY}/manifests")
file(LOCK "${AU_OUTPUT_DIRECTORY}/provenance.lock" GUARD PROCESS TIMEOUT 15)
set(AU_MANIFEST "${AU_OUTPUT_DIRECTORY}/manifests/${AU_FACTS_HASH}.json")
set(AU_MANIFEST_DIGEST "${AU_MANIFEST}.sha256")

if(EXISTS "${AU_MANIFEST}" OR EXISTS "${AU_MANIFEST_DIGEST}")
    if(NOT EXISTS "${AU_MANIFEST}" OR NOT EXISTS "${AU_MANIFEST_DIGEST}")
        message(FATAL_ERROR "Incomplete immutable build manifest; preserve it and use a fresh build directory")
    endif()
    file(SHA256 "${AU_MANIFEST}" AU_ACTUAL_MANIFEST_HASH)
    file(READ "${AU_MANIFEST_DIGEST}" AU_RECORDED_MANIFEST_HASH)
    string(STRIP "${AU_RECORDED_MANIFEST_HASH}" AU_RECORDED_MANIFEST_HASH)
    if(NOT AU_ACTUAL_MANIFEST_HASH STREQUAL AU_RECORDED_MANIFEST_HASH)
        message(FATAL_ERROR "Immutable build manifest bytes changed")
    endif()
    file(READ "${AU_MANIFEST}" AU_MANIFEST_CONTENT)
    foreach(key IN ITEMS schemaVersion version versionLabel buildNumber buildId sourceRevision sourceTree timestampKind)
        string(JSON expected GET "${AU_FACTS}" "${key}")
        string(JSON actual ERROR_VARIABLE error GET "${AU_MANIFEST_CONTENT}" "${key}")
        if(NOT error STREQUAL "NOTFOUND" OR NOT actual STREQUAL expected)
            message(FATAL_ERROR "Immutable build manifest no longer matches configured metadata: ${key}")
        endif()
    endforeach()
    string(JSON recordedTime ERROR_VARIABLE error GET "${AU_MANIFEST_CONTENT}" buildStartedAtUtc)
    if(NOT error STREQUAL "NOTFOUND" OR NOT recordedTime MATCHES "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z$")
        message(FATAL_ERROR "Immutable build manifest lacks its recorded UTC build-start time")
    endif()
else()
    # Record the build producer's time once. SOURCE_DATE_EPOCH must not turn a
    # source-commit date into a timestamp described as the time this build began.
    set(had_epoch FALSE)
    if(DEFINED ENV{SOURCE_DATE_EPOCH})
        set(had_epoch TRUE)
        set(saved_epoch "$ENV{SOURCE_DATE_EPOCH}")
        unset(ENV{SOURCE_DATE_EPOCH})
    endif()
    string(TIMESTAMP AU_BUILD_STARTED_AT "%Y-%m-%dT%H:%M:%SZ" UTC)
    if(had_epoch)
        set(ENV{SOURCE_DATE_EPOCH} "${saved_epoch}")
    endif()
    string(JSON AU_MANIFEST_CONTENT SET "${AU_FACTS}" buildStartedAtUtc "\"${AU_BUILD_STARTED_AT}\"")
    string(APPEND AU_MANIFEST_CONTENT "\n")
    string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef transaction)
    file(WRITE "${AU_MANIFEST}.${transaction}.tmp" "${AU_MANIFEST_CONTENT}")
    file(RENAME "${AU_MANIFEST}.${transaction}.tmp" "${AU_MANIFEST}")
    file(SHA256 "${AU_MANIFEST}" AU_ACTUAL_MANIFEST_HASH)
    file(WRITE "${AU_MANIFEST_DIGEST}.${transaction}.tmp" "${AU_ACTUAL_MANIFEST_HASH}\n")
    file(RENAME "${AU_MANIFEST_DIGEST}.${transaction}.tmp" "${AU_MANIFEST_DIGEST}")
endif()

# Embed exactly the recorded JSON bytes without interpolating them as C++ code.
file(READ "${AU_MANIFEST}" AU_MANIFEST_HEX HEX)
string(CONCAT AU_HEADER_CONTENT "#pragma once\n"
    "#define AU_BUILD_MANIFEST_HEX \"${AU_MANIFEST_HEX}\"\n"
    "#define AU_BUILD_MANIFEST_SHA256 \"${AU_ACTUAL_MANIFEST_HASH}\"\n")
set(AU_EXISTING_HEADER "")
if(EXISTS "${AU_OUTPUT_HEADER}")
    file(READ "${AU_OUTPUT_HEADER}" AU_EXISTING_HEADER)
endif()
if(NOT AU_EXISTING_HEADER STREQUAL AU_HEADER_CONTENT)
    file(WRITE "${AU_OUTPUT_HEADER}.tmp" "${AU_HEADER_CONTENT}")
    file(RENAME "${AU_OUTPUT_HEADER}.tmp" "${AU_OUTPUT_HEADER}")
endif()
