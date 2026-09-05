# Audacity: A Digital Audio Editor
#
# Checks that every commit hash named in CHANGELOG.md is present in this
# repository. A changelog that points at a commit nobody can reach is worse
# than no changelog, so a bad hash fails the configure step.
#
# When the source tree is not a git checkout, for example in a source tarball,
# there is nothing to check against and the check reports a warning and passes.

function(au_validate_changelog changelog_path)
    if (NOT EXISTS "${changelog_path}")
        message(WARNING "No changelog at ${changelog_path}, skipping the commit hash check")
        return()
    endif()

    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/.git")
        message(WARNING "This is not a git checkout, skipping the changelog commit hash check")
        return()
    endif()

    find_program(AU_GIT_EXECUTABLE git)
    if (NOT AU_GIT_EXECUTABLE)
        message(WARNING "git was not found, skipping the changelog commit hash check")
        return()
    endif()

    file(READ "${changelog_path}" changelog_text)
    string(REGEX MATCHALL "[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]"
                  found_hashes "${changelog_text}")

    if (NOT found_hashes)
        message(WARNING "The changelog names no commit hashes, nothing to check")
        return()
    endif()

    list(REMOVE_DUPLICATES found_hashes)

    set(missing_hashes)
    foreach(commit_hash IN LISTS found_hashes)
        execute_process(
            COMMAND ${AU_GIT_EXECUTABLE} cat-file -e "${commit_hash}^{commit}"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE cat_file_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if (NOT cat_file_result EQUAL 0)
            list(APPEND missing_hashes ${commit_hash})
        endif()
    endforeach()

    list(LENGTH found_hashes checked_count)

    if (missing_hashes)
        string(REPLACE ";" "\n  " missing_text "${missing_hashes}")
        message(FATAL_ERROR
            "CHANGELOG.md names commits that are not in this repository:\n  ${missing_text}\n"
            "Every changelog entry must carry the full hash of a commit that exists here.")
    endif()

    message(STATUS "Changelog: ${checked_count} commit hashes checked, all present")
endfunction()
