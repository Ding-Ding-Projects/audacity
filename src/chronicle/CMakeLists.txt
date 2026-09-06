#
# Audacity: A Digital Audio Editor
#

declare_module(chronicle)

set(MODULE_QRC au_chronicle.qrc)

set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/chroniclemodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/chroniclemodule.h
    ${CMAKE_CURRENT_LIST_DIR}/iversionhistoryservice.h

    ${CMAKE_CURRENT_LIST_DIR}/types/chronicletypes.h

    ${CMAKE_CURRENT_LIST_DIR}/internal/isnapshotstore.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/gitsnapshotstore.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/gitsnapshotstore.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/filesnapshotstore.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/filesnapshotstore.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/versionhistoryservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/versionhistoryservice.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/changelogparser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/changelogparser.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/tabstripstate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/tabstripstate.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/chronicleactionscontroller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/chronicleactionscontroller.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/chronicleuiactions.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/chronicleuiactions.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/projecthistorywatcher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/projecthistorywatcher.h

    ${CMAKE_CURRENT_LIST_DIR}/view/versionhistorymodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/versionhistorymodel.h
    ${CMAKE_CURRENT_LIST_DIR}/view/chronicledebughooks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/chronicledebughooks.h
    ${CMAKE_CURRENT_LIST_DIR}/view/changelogmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/changelogmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/view/tabstripmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/tabstripmodel.h
    )

set(MODULE_LINK
    muse::uicomponents_qml
    Qt::Quick
    au3wrap
    )

setup_module()

if (AU_BUILD_CHRONICLE_TESTS)
    add_subdirectory(tests)
endif()
