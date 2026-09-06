# Audacity: A Digital Audio Editor
#
# Runs buildscripts/checks/completeness_guard.py at configure time. The
# guard reads docs/inventory/completeness-inventory.md and reports every
# canonical feature that is missing a row, or that references an
# implementation file, documentation article, test file or capture path
# which does not exist on disk.
#
# Controlled by two options:
#   AU_CHECK_COMPLETENESS_INVENTORY (default ON)  - run the check at all.
#   AU_COMPLETENESS_STRICT (default OFF)          - fail configure on any
#                                                    finding, rather than
#                                                    only printing it.
#
# With no python3 interpreter available the check is skipped with a warning
# rather than failing configure; a missing interpreter is an environment
# fact, not a completeness defect.

option(AU_CHECK_COMPLETENESS_INVENTORY "Check docs/inventory/completeness-inventory.md at configure time" ON)
option(AU_COMPLETENESS_STRICT "Fail configure when the completeness inventory guard finds a problem" OFF)

function(au_check_completeness_inventory)
    if (NOT AU_CHECK_COMPLETENESS_INVENTORY)
        return()
    endif()

    find_package(Python3 QUIET COMPONENTS Interpreter)
    if (NOT Python3_Interpreter_FOUND)
        message(WARNING "python3 was not found, skipping the completeness inventory guard")
        return()
    endif()

    set(guard_script "${CMAKE_CURRENT_LIST_DIR}/../checks/completeness_guard.py")
    if (NOT EXISTS "${guard_script}")
        message(WARNING "Completeness inventory guard script not found at ${guard_script}, skipping")
        return()
    endif()

    set(guard_args --repo-root "${PROJECT_SOURCE_DIR}")
    if (AU_COMPLETENESS_STRICT)
        list(APPEND guard_args --strict)
    endif()

    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${guard_script}" ${guard_args}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE guard_result
        OUTPUT_VARIABLE guard_output
        ERROR_VARIABLE guard_error
    )

    if (guard_output)
        message(STATUS "${guard_output}")
    endif()

    if (NOT guard_result EQUAL 0)
        if (AU_COMPLETENESS_STRICT)
            message(FATAL_ERROR "Completeness inventory guard failed (AU_COMPLETENESS_STRICT is ON):\n${guard_error}")
        else()
            message(WARNING "Completeness inventory guard found problems (set AU_COMPLETENESS_STRICT=ON to fail configure on this):\n${guard_error}")
        endif()
    endif()
endfunction()

au_check_completeness_inventory()

# Runs buildscripts/checks/material_guard.py at configure time. The guard
# scans every src/**/qml/**/*.qml file for legacy Muse control names,
# hard coded colour and radius literals and ui.theme.* colour reads outside
# the Audacity.M3 library, and fails on any hit that is not allow listed
# with a reason in docs/inventory/material-audit.md. It shares the
# AU_COMPLETENESS_STRICT option above: OFF (default) prints a warning,
# ON fails configure.

option(AU_CHECK_MATERIAL_AUDIT "Check docs/inventory/material-audit.md against the QML tree at configure time" ON)

function(au_check_material_audit)
    if (NOT AU_CHECK_MATERIAL_AUDIT)
        return()
    endif()

    find_package(Python3 QUIET COMPONENTS Interpreter)
    if (NOT Python3_Interpreter_FOUND)
        message(WARNING "python3 was not found, skipping the Material Design 3 audit guard")
        return()
    endif()

    set(guard_script "${CMAKE_CURRENT_LIST_DIR}/../checks/material_guard.py")
    if (NOT EXISTS "${guard_script}")
        message(WARNING "Material Design 3 audit guard script not found at ${guard_script}, skipping")
        return()
    endif()

    set(guard_args --repo-root "${PROJECT_SOURCE_DIR}")
    if (AU_COMPLETENESS_STRICT)
        list(APPEND guard_args --strict)
    endif()

    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${guard_script}" ${guard_args}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE guard_result
        OUTPUT_VARIABLE guard_output
        ERROR_VARIABLE guard_error
    )

    if (guard_output)
        message(STATUS "${guard_output}")
    endif()

    if (NOT guard_result EQUAL 0)
        if (AU_COMPLETENESS_STRICT)
            message(FATAL_ERROR "Material Design 3 audit guard failed (AU_COMPLETENESS_STRICT is ON):\n${guard_error}")
        else()
            message(WARNING "Material Design 3 audit guard found problems (set AU_COMPLETENESS_STRICT=ON to fail configure on this):\n${guard_error}")
        endif()
    endif()
endfunction()

au_check_material_audit()
