muse_create_qml_module(appshell_qml FOR appshell)

qt_add_qml_module(appshell_qml
    URI Audacity.AppShell
    VERSION 1.0
    SOURCES
        aboutmodel.cpp
        aboutmodel.h
        appmenumodel.cpp
        appmenumodel.h
        DevTools/Settings/settingslistmodel.cpp
        DevTools/Settings/settingslistmodel.h
        DevTools/Table/tabletestsmodel.cpp
        DevTools/Table/tabletestsmodel.h
        FirstLaunchSetup/clipvisualizationpagemodel.cpp
        FirstLaunchSetup/clipvisualizationpagemodel.h
        FirstLaunchSetup/firstlaunchsetupmodel.cpp
        FirstLaunchSetup/firstlaunchsetupmodel.h
        FirstLaunchSetup/languagepagemodel.cpp
        FirstLaunchSetup/languagepagemodel.h
        FirstLaunchSetup/themespagemodel.cpp
        FirstLaunchSetup/themespagemodel.h
        FirstLaunchSetup/workspacelayoutpagemodel.cpp
        FirstLaunchSetup/workspacelayoutpagemodel.h
        FirstLaunchSetup/signinaudiocompagemodel.cpp
        FirstLaunchSetup/signinaudiocompagemodel.h
        FirstLaunchSetup/appupdateusageinfopagemodel.cpp
        FirstLaunchSetup/appupdateusageinfopagemodel.h
        framelesswindowmodel.cpp
        framelesswindowmodel.h
        maintoolbarmodel.cpp
        maintoolbarmodel.h
        mainwindowtitleprovider.cpp
        mainwindowtitleprovider.h
        projectpagemodel.cpp
        projectpagemodel.h
        PublishPage/publishtoolbarmodel.cpp
        PublishPage/publishtoolbarmodel.h
        windowdroparea.cpp
        windowdroparea.h
        welcomedialogmodel.cpp
        welcomedialogmodel.h
    QML_FILES
        AboutDialog.qml
        AboutDialogAudacityTab.qml
        AboutDialogPrivacyTab.qml
        AlphaWelcomePopup.qml
        AppWindow.qml
        DevTools/CrashHandler/CrashHandlerDevTools.qml
        DevTools/DevToolsMenu.qml
        DevTools/DevToolsPage.qml
        DevTools/Extensions/ExtensionsListView.qml
        DevTools/Gallery/GeneralComponentsGallery.qml
        DevTools/Gallery/M3ComponentsGallery.qml
        DevTools/Interactive/InteractiveTests.qml
        DevTools/Interactive/SampleDialog.qml
        DevTools/KeyNav/KeyNavExample.qml
        DevTools/KeyNav/KeyNavSection.qml
        DevTools/KeyNav/KeyNavSubSection.qml
        DevTools/Preferences/SettingsPage.qml
        DevTools/Table/TableTests.qml
        FirstLaunchSetup/ClipVisualizationPage.qml
        FirstLaunchSetup/LanguagePage.qml
        FirstLaunchSetup/SeedColorPage.qml
        FirstLaunchSetup/DoublePage.qml
        FirstLaunchSetup/FirstLaunchSetupDialog.qml
        FirstLaunchSetup/Page.qml
        FirstLaunchSetup/PlaybackPage.qml
        FirstLaunchSetup/ThemesPage.qml
        FirstLaunchSetup/TutorialsPage.qml
        FirstLaunchSetup/UnderConstructionPage.qml
        FirstLaunchSetup/WorkspaceLayoutPage.qml
        FirstLaunchSetup/SigninAudiocomPage.qml
        FirstLaunchSetup/AppUpdatesAndUsageInfoPage.qml
        SigninAudiocomDialog.qml
        HomePage/CloudItem.qml
        HomePage/HomeMenu.qml
        HomePage/HomePage.qml
        HomePage/PluginsPage.qml
        M3AppTitleBar.qml
        Main.qml
        Main.wasm.qml
        MainToolBar.qml
        platform/AppMenuBar.qml
        platform/PlatformMenuBar.qml
        ProjectPage/ProjectPage.qml
        ProjectPage/ProjectStatusBar.qml
        PublishPage/PublishPage.qml
        PublishPage/PublishToolBar.qml
        shared/AccentColorsList.qml
        shared/ThemeSamplesList.qml
        shared/internal/ThemeSample.qml
        WelcomeDialog.qml
        WindowContent.qml
    RESOURCES
        FirstLaunchSetup/resources/ClipVisuals_ClassicClips.png
        FirstLaunchSetup/resources/ClipVisuals_ColorfulClips.png
        FirstLaunchSetup/resources/MuseSounds.png
        FirstLaunchSetup/resources/UnderConstruction.png
        FirstLaunchSetup/resources/VideoTutorials.png
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
        TARGET muse_cloud_qml
        TARGET muse_dockwindow_qml
        TARGET muse_diagnostics_qml
        TARGET muse_extensions_qml
        TARGET muse_learn_qml
        TARGET muse_shortcuts_qml
        TARGET muse_toast_qml
        TARGET muse_tours_qml
)

if (AU_BUILD_CLOUD_AUDIOCOM)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            HomePage/AccountPage.qml
    )
endif()

if (OS_IS_MAC)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            platform/mac/Main.qml
    )
elseif(OS_IS_WIN)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            platform/win/AppTitleBar.qml
            platform/win/Main.qml
    )
elseif(OS_IS_LIN OR OS_IS_FREEBSD)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            platform/linux/Main.qml
    )
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

if (NOT OS_IS_MAC)
    target_sources(appshell_qml PRIVATE
        navigableappmenumodel.cpp
        navigableappmenumodel.h
    )

    # we use QKeyMapper to fix https://github.com/musescore/MuseScore/issues/10181
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
    target_link_libraries(appshell_qml PRIVATE Qt6::GuiPrivate)
endif(NOT OS_IS_MAC)

target_link_libraries(appshell_qml PRIVATE Qt::Widgets)

fixup_qml_module_dependencies(appshell_qml)

# Necessary for the auto-generated sources
target_include_directories(appshell_qml PRIVATE
    DevTools/Settings
    DevTools/Table
    FirstLaunchSetup
    PublishPage
)

# Generate provenance when this candidate is built, not when CMake happened to
# configure it. The generation script checks that HEAD still matches this
# configured candidate and fails instead of attributing a later tree to it.
execute_process(
    COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --verify HEAD
    OUTPUT_VARIABLE AU_CONFIGURED_SOURCE_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE AU_CONFIGURED_SOURCE_REVISION_RESULT
)
if (NOT AU_CONFIGURED_SOURCE_REVISION_RESULT EQUAL 0)
    message(FATAL_ERROR "Build provenance requires a Git candidate revision")
endif()

set(AU_BUILD_PROVENANCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(AU_BUILD_PROVENANCE_HEADER "${AU_BUILD_PROVENANCE_DIR}/appshellbuildprovenance.h")
add_custom_target(appshell_build_provenance ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DAU_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DAU_EXPECTED_SOURCE_REVISION=${AU_CONFIGURED_SOURCE_REVISION}"
        "-DAU_OUTPUT_HEADER=${AU_BUILD_PROVENANCE_HEADER}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/GenerateBuildProvenance.cmake"
    BYPRODUCTS "${AU_BUILD_PROVENANCE_HEADER}"
    VERBATIM
)
add_dependencies(appshell_qml appshell_build_provenance)
target_include_directories(appshell_qml PRIVATE "${AU_BUILD_PROVENANCE_DIR}")

if (DEFINED MUSE_APP_VERSION)
    target_compile_definitions(appshell_qml PRIVATE
        MUSE_APP_VERSION="${MUSE_APP_VERSION}"
    )
endif()
