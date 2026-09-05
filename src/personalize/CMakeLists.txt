#
# Audacity: A Digital Audio Editor
#

declare_module(personalize)

set(MODULE_QRC au_personalize.qrc)

set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/personalizemodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/personalizemodule.h

    ${CMAKE_CURRENT_LIST_DIR}/internal/appearanceoverrides.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/appearanceoverrides.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/displaynamesettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/displaynamesettings.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/lockregistry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/lockregistry.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/mutationhistory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/mutationhistory.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/qrencoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/qrencoder.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/qrcodemodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/qrcodemodel.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/totpengine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/totpengine.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/authenticatorstore.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/authenticatorstore.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/authenticatormodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/authenticatormodel.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/supporttickets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/supporttickets.h
)

set(MODULE_LINK
    muse::uicomponents_qml
    muse::ui
    muse::global
)

setup_module()

if (AU_BUILD_PERSONALIZE_TESTS)
    add_subdirectory(tests)
endif()
