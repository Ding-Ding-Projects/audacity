#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST toolkit_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/hardwarefit_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/harnessprofile_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/exportservice_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/bulkselectionmodel_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pullcart_tests.cpp
)

set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

set(MODULE_TEST_LINK
    toolkit

    muse::uicomponents
    muse::ui

    Qt::Quick
    Qt::Network
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})
include(SetupGTest)
