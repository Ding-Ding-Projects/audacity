#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST squirrelupdate_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/releasesparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/packageverifier_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/squirrelinstalllayout_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/updatebannerstate_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/restartcoordinator_tests.cpp
    )

set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

set(MODULE_TEST_LINK
    squirrelupdate
    Qt6::Core
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})

include(SetupGTest)
set_target_properties(${MODULE_TEST} PROPERTIES AUTOMOC ON)
