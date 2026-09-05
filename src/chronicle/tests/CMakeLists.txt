#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST chronicle_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/snapshotstore_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/changelog_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tabstrip_tests.cpp
)

set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

set(MODULE_TEST_LINK
    chronicle

    muse::global
    Qt::Core
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})
include(SetupGTest)
