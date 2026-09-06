#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST experience_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/messagestyler_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/personalvocabulary_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/settingsscheduler_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/experiencetranslator_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dimsumsurprise_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/schoolmode_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/narratorqueue_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/notificationcenter_tests.cpp
    )

set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

set(MODULE_TEST_LINK
    experience
    Qt6::Core
    Qt6::Network
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})

include(SetupGTest)
set_target_properties(${MODULE_TEST} PROPERTIES AUTOMOC ON)
