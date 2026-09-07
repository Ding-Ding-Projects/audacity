#
# Audacity: A Digital Audio Editor
#
# This module is deliberately not registered from src/CMakeLists.txt yet.  The
declare_module(converter)

set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/convertercatalog.h
    ${CMAKE_CURRENT_LIST_DIR}/convertercatalog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/conversionengine.h
    ${CMAKE_CURRENT_LIST_DIR}/conversionengine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/processcontainment.h
    ${CMAKE_CURRENT_LIST_DIR}/nativefiletransaction.h
    ${CMAKE_CURRENT_LIST_DIR}/pdfprocessor.h
    ${CMAKE_CURRENT_LIST_DIR}/pdfprocessor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/conversionqueue.h
    ${CMAKE_CURRENT_LIST_DIR}/conversionqueue.cpp
    ${CMAKE_CURRENT_LIST_DIR}/convertermodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/convertermodule.h
    ${CMAKE_CURRENT_LIST_DIR}/view/converterpresentationmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/converterpresentationmodel.h
)

set(MODULE_LINK
    Qt::Gui
    Qt::Concurrent
)

setup_module()

add_subdirectory(qml/Audacity/Converter)

if (AU_BUILD_CONVERTER_TESTS)
    add_subdirectory(tests)
endif()
