
message(STATUS "Package")

# Config
set(ARTIFACTS_DIR "build.artifacts")
set(BUILD_DIR "build.release")
set(INSTALL_DIR "build.install")

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

# Setup package type.
# devel and nightly builds ship a plain archive; testing and stable builds ship
# the Squirrel.Windows installer. There is no MSI, WiX or other fallback.
set(PACK_TYPE "7z")
if (BUILD_MODE STREQUAL "devel_build")
  set(PACK_TYPE "7z")
elseif(BUILD_MODE STREQUAL "nightly_build")
  set(PACK_TYPE "7z")
elseif(BUILD_MODE STREQUAL "testing_build")
  set(PACK_TYPE "squirrel")
elseif(BUILD_MODE STREQUAL "stable_build")
  set(PACK_TYPE "squirrel")
endif()

if (PACK_TYPE_OVERRIDE)
  set(PACK_TYPE "${PACK_TYPE_OVERRIDE}")
endif()

if (PACKAGE_DRY_RUN)
    message(STATUS "PACKAGE_DRY_RUN is ON, nothing to do")
    return()
endif()

file(MAKE_DIRECTORY "${ARTIFACTS_DIR}")

# PACK 7z
if(PACK_TYPE STREQUAL "7z")
  message(STATUS "Start 7z packing...")
  set(ARTIFACT_NAME "Audacity-${BUILD_VERSION}-${PACKARCH}")

  file(RENAME ${INSTALL_DIR} ${ARTIFACT_NAME})
  file(ARCHIVE_CREATE OUTPUT ${ARTIFACTS_DIR}/${ARTIFACT_NAME}.7z PATHS ${ARTIFACT_NAME} FORMAT 7zip)

  message(STATUS "Finished 7z packing")
  return()
endif()

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
      -Tag "${RELEASE_TAG}"
      -RunNumber "${RUN_NUMBER}"
  )

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
