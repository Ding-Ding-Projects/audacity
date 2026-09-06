muse_create_qml_module(experience_qml FOR experience)

set_source_files_properties(ExperienceBridge.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(experience_qml
    URI Audacity.Experience
    VERSION 1.0
    SOURCES
        dimsumsurprisemodel.cpp
        dimsumsurprisemodel.h
        experiencesettingsmodel.cpp
        experiencesettingsmodel.h
        notificationlistmodel.cpp
        notificationlistmodel.h
        schedulelistmodel.cpp
        schedulelistmodel.h
    QML_FILES
        DimSumSurpriseCard.qml
        ExperienceBridge.qml
        ExperienceOverlay.qml
        ExperienceToast.qml
        NotificationCentre.qml
        NotificationHost.qml
        ScheduleEditSheet.qml
        SuperConfirmationDialog.qml
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
)

fixup_qml_module_dependencies(experience_qml)

target_link_libraries(experience_qml PRIVATE muse::interactive muse::ui)
