#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST au_au3wrap_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/tracktemplatefactory.h
    ${CMAKE_CURRENT_LIST_DIR}/tracktemplatefactory.cpp

    ${CMAKE_CURRENT_LIST_DIR}/mocks/au3projectmock.h

    ${CMAKE_CURRENT_LIST_DIR}/projectmetadata_tests.cpp
)

set(MODULE_TEST_LINK
    au3wrap
    sqlite
    au3-sqlite-helpers
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})

include(SetupGTest)
