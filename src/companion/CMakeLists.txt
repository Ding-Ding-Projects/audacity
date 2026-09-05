#
# Audacity: A Digital Audio Editor
#

declare_module(companion)

set(MODULE_QRC au_companion.qrc)

set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/companionmodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/companionmodule.h

    ${CMAKE_CURRENT_LIST_DIR}/internal/companionuiactions.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/companionuiactions.h

    ${CMAKE_CURRENT_LIST_DIR}/palette/commandpalettemodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/palette/commandpalettemodel.h
    ${CMAKE_CURRENT_LIST_DIR}/palette/settingsindex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/palette/settingsindex.h

    ${CMAKE_CURRENT_LIST_DIR}/regex/regexengine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/regex/regexengine.h
)

set(MODULE_LINK
    muse::uicomponents_qml
    muse::ui
)

# The palette indexes the repository's own documentation when it can see it,
# which is what a development build wants. An installed build falls back to the
# copy that sits beside the application.
set(MODULE_DEF
    -DAU_COMPANION_DOCS_DIR="${CMAKE_CURRENT_LIST_DIR}/../../docs"
)

setup_module()

if (AU_BUILD_COMPANION_TESTS)
    add_subdirectory(tests)
endif()
