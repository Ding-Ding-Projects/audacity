include(GetPlatformInfo)

if(NOT OS_IS_WIN)
    return()
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME ${MUSE_APP_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Audacity is a digital audio editor")
set(CPACK_PACKAGE_VENDOR "Audacity")
set(CPACK_PACKAGE_CONTACT "https://audacityteam.org")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://audacityteam.org")

set(CPACK_PACKAGE_VERSION_MAJOR "${MUSE_APP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MUSE_APP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MUSE_APP_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_BUILD "${CMAKE_BUILD_NUMBER}")
set(CPACK_PACKAGE_VERSION "${MUSE_APP_VERSION_MAJOR}.${MUSE_APP_VERSION_MINOR}.${MUSE_APP_VERSION_PATCH}.${CPACK_PACKAGE_VERSION_BUILD}")
message("CPACK_PACKAGE_VERSION: ${CPACK_PACKAGE_VERSION}")

set(git_date_string "")

if(MUSE_APP_UNSTABLE)
    find_program(GIT_EXECUTABLE git PATHS ENV PATH)

    if(GIT_EXECUTABLE)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" log -1 --date=short --format=%cd
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE git_date
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    if(git_date)
        string(REGEX REPLACE "-" "" git_date "${git_date}")
        set(git_date_string "~git${git_date}")
    endif()
endif(MUSE_APP_UNSTABLE)

set(CPACK_PACKAGE_FILE_NAME "${MUSE_APP_NAME}-${MUSE_APP_VERSION}${git_date_string}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${MUSE_APP_NAME_VERSION})

# Packaging generator
#
# The Windows installer for this project is produced exclusively by
# Squirrel.Windows through buildscripts/ci/windows/package_squirrel.ps1.
# CPack is kept only for the plain archive used by developer builds, so the
# WiX generator, its templates and every code signing hook have been removed.
# Code signing is permanently prohibited: do not reintroduce signtool here.
set(CPACK_GENERATOR "ZIP")

set(MUSE_EXECUTABLE_NAME ${MUSE_APP_NAME}${MUSE_APP_VERSION_MAJOR})

message(STATUS "========== Audacity Packaging Variables ==========")
  message(STATUS "MUSE_APP_TITLE='${MUSE_APP_TITLE}'")
  message(STATUS "MUSE_APP_TITLE_VERSION='${MUSE_APP_TITLE_VERSION}'")
  message(STATUS "MUSE_APP_RELEASE_CHANNEL='${MUSE_APP_RELEASE_CHANNEL}'")
  message(STATUS "MUSE_EXECUTABLE_NAME='${MUSE_EXECUTABLE_NAME}'")
  message(STATUS "CPACK_PACKAGE_NAME='${CPACK_PACKAGE_NAME}'")
  message(STATUS "CPACK_PACKAGE_INSTALL_DIRECTORY='${CPACK_PACKAGE_INSTALL_DIRECTORY}'")
  message(STATUS "CPACK_PACKAGE_VERSION_MAJOR='${CPACK_PACKAGE_VERSION_MAJOR}'")
  message(STATUS "CPACK_PACKAGE_VERSION_MINOR='${CPACK_PACKAGE_VERSION_MINOR}'")
  message(STATUS "CPACK_PACKAGE_VERSION_PATCH='${CPACK_PACKAGE_VERSION_PATCH}'")
  message(STATUS "CPACK_PACKAGE_EXECUTABLES='${CPACK_PACKAGE_EXECUTABLES}'")
  message(STATUS "CPACK_CREATE_DESKTOP_LINKS='${CPACK_CREATE_DESKTOP_LINKS}'")
  message(STATUS "==================================================")


include(CPack)
