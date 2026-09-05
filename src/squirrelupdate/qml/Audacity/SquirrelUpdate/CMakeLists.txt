muse_create_qml_module(squirrelupdate_qml FOR squirrelupdate)

qt_add_qml_module(squirrelupdate_qml
    URI Audacity.SquirrelUpdate
    VERSION 1.0
    SOURCES
        squirrelupdatemodel.cpp
        squirrelupdatemodel.h
    QML_FILES
        UpdateBanner.qml
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
)

fixup_qml_module_dependencies(squirrelupdate_qml)
