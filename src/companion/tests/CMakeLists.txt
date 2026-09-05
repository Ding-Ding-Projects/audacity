#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST companion_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/regexengine_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/search_inventory_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/palette_index_tests.cpp
)

set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

# The guard tests scan the source tree rather than the built resources, so they
# are given the roots they need at configure time.
set(MODULE_TEST_DEF
    -DAU_COMPANION_SOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}/../.."
    -DAU_COMPANION_REPO_ROOT="${CMAKE_CURRENT_LIST_DIR}/../../.."
    -DAU_COMPANION_FRAMEWORK_ROOT="${CMAKE_CURRENT_LIST_DIR}/../../../muse/framework"
)

set(MODULE_TEST_LINK
    companion

    muse::uicomponents
    muse::ui

    Qt::Quick
    )

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})
include(SetupGTest)
