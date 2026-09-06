# Converter tests are kept with the unregistered module so application wiring
# can add this directory at the same time it enables AU_BUILD_CONVERTER_TESTS.
set(MODULE_TEST converter_tests)
set(MODULE_TEST_SRC ${CMAKE_CURRENT_LIST_DIR}/conversionengine_tests.cpp)
set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)
set(MODULE_TEST_LINK converter Qt::Gui)
set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})
include(SetupGTest)
