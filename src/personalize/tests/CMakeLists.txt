#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST personalize_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/totpengine_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qrencoder_tests.cpp
)

set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

set(MODULE_TEST_LINK
    personalize

    muse::uicomponents
    muse::ui

    Qt::Quick
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})
include(SetupGTest)
