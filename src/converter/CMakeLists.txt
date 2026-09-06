#
# Audacity: A Digital Audio Editor
#
# This module is deliberately not registered from src/CMakeLists.txt yet.  The
# application integration lane owns registration, QML exposure and packaging
# proof.  Keeping this file complete lets that lane add one add_subdirectory
# after it has supplied those pieces.

declare_module(converter)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/convertercatalog.h
    ${CMAKE_CURRENT_LIST_DIR}/convertercatalog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/conversionengine.h
    ${CMAKE_CURRENT_LIST_DIR}/conversionengine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/conversionqueue.h
    ${CMAKE_CURRENT_LIST_DIR}/conversionqueue.cpp
)

set(MODULE_LINK
    Qt::Gui
)

setup_module()

if (AU_BUILD_CONVERTER_TESTS)
    add_subdirectory(tests)
endif()
