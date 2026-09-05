# **********************************************************************
#
#  Audacity: A Digital Audio Editor
#
#  Material Design 3 patch overlay for the muse submodule.
#
#  Audacity cannot fork the MuseScore "muse" framework, so the Material
#  Design 3 changes to the parts of the interface that muse draws itself
#  live as numbered patches in buildscripts/muse-patches. They are applied
#  to the submodule working tree at configure time. Applying is idempotent:
#  a patch that is already applied is detected and skipped, so re-running
#  CMake never fails and never applies a patch twice.
#
#  See docs/design/MUSE_OVERLAY.md and buildscripts/muse-patches/README.md.
#
# **********************************************************************

option(AU_APPLY_MUSE_PATCHES "Apply the Material Design 3 patch overlay to the muse submodule" ON)

function(au_apply_muse_patches)
    if(NOT AU_APPLY_MUSE_PATCHES)
        message(STATUS "muse patch overlay: disabled (AU_APPLY_MUSE_PATCHES is OFF)")
        return()
    endif()

    set(_patch_dir ${CMAKE_CURRENT_LIST_DIR}/../muse-patches)
    cmake_path(NORMAL_PATH _patch_dir)

    if(NOT EXISTS ${MUSE_FRAMEWORK_PATH}/.git)
        message(STATUS "muse patch overlay: skipped, ${MUSE_FRAMEWORK_PATH} is not a git working tree")
        return()
    endif()

    find_package(Git QUIET)
    if(NOT Git_FOUND)
        message(FATAL_ERROR
            "muse patch overlay: git was not found, but it is required to apply "
            "the Material Design 3 patches in ${_patch_dir}. "
            "Configure with -DAU_APPLY_MUSE_PATCHES=OFF to skip the overlay.")
    endif()

    file(GLOB _patches ${_patch_dir}/*.patch)
    list(SORT _patches)

    if(NOT _patches)
        message(STATUS "muse patch overlay: no patches found in ${_patch_dir}")
        return()
    endif()

    foreach(_patch ${_patches})
        get_filename_component(_name ${_patch} NAME)

        # A patch that reverses cleanly is already applied.
        execute_process(
            COMMAND ${GIT_EXECUTABLE} -C ${MUSE_FRAMEWORK_PATH} apply --check --reverse -p1 ${_patch}
            RESULT_VARIABLE _reverse_result
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(_reverse_result EQUAL 0)
            message(STATUS "muse patch overlay: ${_name} already applied")
            continue()
        endif()

        execute_process(
            COMMAND ${GIT_EXECUTABLE} -C ${MUSE_FRAMEWORK_PATH} apply --check -p1 ${_patch}
            RESULT_VARIABLE _check_result
            ERROR_VARIABLE _check_error
            OUTPUT_QUIET
        )

        if(NOT _check_result EQUAL 0)
            message(FATAL_ERROR
                "muse patch overlay: ${_name} does not apply to ${MUSE_FRAMEWORK_PATH}.\n"
                "${_check_error}\n"
                "The muse submodule is probably at a different commit than the one the "
                "patches were generated against. Update the patches with "
                "'python3 buildscripts/tools/muse_patches.py regenerate', or configure with "
                "-DAU_APPLY_MUSE_PATCHES=OFF to build without the Material Design 3 overlay.")
        endif()

        execute_process(
            COMMAND ${GIT_EXECUTABLE} -C ${MUSE_FRAMEWORK_PATH} apply -p1 ${_patch}
            RESULT_VARIABLE _apply_result
            ERROR_VARIABLE _apply_error
        )

        if(NOT _apply_result EQUAL 0)
            message(FATAL_ERROR "muse patch overlay: failed to apply ${_name}.\n${_apply_error}")
        endif()

        message(STATUS "muse patch overlay: applied ${_name}")
    endforeach()
endfunction()

au_apply_muse_patches()
