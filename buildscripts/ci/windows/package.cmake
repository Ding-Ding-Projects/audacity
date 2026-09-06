
message(STATUS "Package")

# Config
set(ARTIFACTS_DIR "build.artifacts")
set(BUILD_DIR "build.release")
set(INSTALL_DIR "build.install" CACHE PATH "Existing installed application payload")

# Options
set(BUILD_MODE "" CACHE STRING "Build mode")
set(PACKARCH "x86_64" CACHE STRING "Package architecture")
set(BUILD_VERSION "" CACHE STRING "Build version")
set(BUILD_NUMBER   "" CACHE STRING "Nightly build number")
set(BUILD_BRANCH   "" CACHE STRING "Nightly branch")
set(BUILD_REVISION "" CACHE STRING "Nightly revision (short SHA)")

# Squirrel.Windows options
set(RELEASE_TAG "" CACHE STRING "Release tag, for example v4.0.0-m3.1")
set(RUN_NUMBER "0" CACHE STRING "CI run number, used when no tag is present")
set(PREVIOUS_RELEASES_DIR "" CACHE STRING "Directory with a previous RELEASES file and .nupkg files")
set(SQUIRREL_OUT_DIR "${ARTIFACTS_DIR}/squirrel" CACHE STRING "Squirrel release directory")

# Set to ON to parse this script without doing any work. Used to check that the
# file still parses on machines that cannot run the packaging itself.
option(PACKAGE_DRY_RUN "Parse only, do nothing" OFF)

# Code signing is permanently prohibited for this project. There is no
# SIGN_KEY, SIGN_SECRET or signtool path here, and none may be added.

if (NOT PACKAGE_DRY_RUN)
    if (NOT BUILD_MODE)
        file (STRINGS "${ARTIFACTS_DIR}/env/build_mode.env" BUILD_MODE)
    endif()

    if (NOT BUILD_MODE)
        message(FATAL_ERROR "not set BUILD_MODE")
    endif()

    if (NOT BUILD_VERSION)
        file (STRINGS "${ARTIFACTS_DIR}/env/build_version.env" BUILD_VERSION)
    endif()
endif()

# Every supported delivery mode produces the same genuine installer format.
set(_supported_modes devel_build nightly_build testing_build stable_build)
list(FIND _supported_modes "${BUILD_MODE}" _mode_index)
if(NOT PACKAGE_DRY_RUN AND _mode_index LESS 0)
  message(FATAL_ERROR "Unsupported Windows package build mode: ${BUILD_MODE}")
endif()
set(PACK_TYPE "squirrel")
if(PACK_TYPE_OVERRIDE AND NOT PACK_TYPE_OVERRIDE STREQUAL "squirrel")
  message(FATAL_ERROR "Only genuine Squirrel.Windows packages are supported; archive-only overrides are prohibited")
endif()

if (PACKAGE_DRY_RUN)
    message(STATUS "PACKAGE_DRY_RUN is ON, nothing to do")
    return()
endif()

file(MAKE_DIRECTORY "${ARTIFACTS_DIR}")

# Repair/verify the installed tool bundle again immediately before any package
# consumes it, including when packaging is invoked without the build script.
get_filename_component(INSTALL_DIR "${INSTALL_DIR}" ABSOLUTE)
set(QPDF_INSTALL_ROOT "${INSTALL_DIR}")
include("${CMAKE_CURRENT_LIST_DIR}/../../converter-tools/provision-qpdf.cmake")

# PACK Squirrel.Windows
if(PACK_TYPE STREQUAL "squirrel")
  message(STATUS "Start Squirrel.Windows packing...")

  if(NOT EXISTS "${INSTALL_DIR}")
    message(FATAL_ERROR "Install tree not found: ${INSTALL_DIR}")
  endif()

  set(SQUIRREL_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/package_squirrel.ps1")
  if(NOT EXISTS "${SQUIRREL_SCRIPT}")
    message(FATAL_ERROR "Squirrel packaging script not found: ${SQUIRREL_SCRIPT}")
  endif()

  find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell)
  if(NOT POWERSHELL_EXECUTABLE)
    message(FATAL_ERROR "PowerShell not found; it is required for Squirrel.Windows packaging")
  endif()

  set(_squirrel_args
      -NoProfile -ExecutionPolicy Bypass
      -File "${SQUIRREL_SCRIPT}"
      -InstallDir "${INSTALL_DIR}"
      -OutDir "${SQUIRREL_OUT_DIR}"
      -RunNumber "${RUN_NUMBER}"
  )

  # CMake list expansion drops empty arguments. Omit the optional parameter
  # entirely so tagless workflow dispatches reach the RunNumber fallback.
  if(NOT RELEASE_TAG STREQUAL "")
    list(APPEND _squirrel_args -Tag "${RELEASE_TAG}")
  endif()

  if(PREVIOUS_RELEASES_DIR)
    list(APPEND _squirrel_args -PreviousReleasesDir "${PREVIOUS_RELEASES_DIR}")
  endif()

  execute_process(
    COMMAND "${POWERSHELL_EXECUTABLE}" ${_squirrel_args}
    RESULT_VARIABLE _squirrel_rc
  )

  if(NOT _squirrel_rc EQUAL 0)
    message(FATAL_ERROR "Squirrel.Windows packaging failed (exit ${_squirrel_rc})")
  endif()

  message(STATUS "Finished Squirrel.Windows packing, output in ${SQUIRREL_OUT_DIR}")
  return()
endif()

message(FATAL_ERROR "Unknown PACK_TYPE: ${PACK_TYPE}")
