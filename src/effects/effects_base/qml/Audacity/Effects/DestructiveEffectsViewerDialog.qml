/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.Effects
import Audacity.BuiltinEffects
import Audacity.Lv2
import Audacity.AudioUnit
import Audacity.Vst
import Audacity.M3

EffectStyledDialogView {
    id: root

    property int effectFamily: EffectFamily.Unknown

    title: viewerModel.title
    navigationSection.name: title

    openPolicies: DialogView.OpenOnContentReady
    isContentReady: {
        if (viewerModel.viewerComponentType !== ViewerComponentType.Generated) {
            return true;
        }

        return viewerLoader.item ? viewerLoader.item.isContentReady : false;
    }

    contentWidth: Math.max(viewerLoader.width + prv.viewMargins * 2, prv.minimumWidth)
    contentHeight: {
        let height = 0;
        height += prv.showTopPanel ? topPanel.height : prv.viewMargins;
        height += viewerLoader.height;
        height += prv.showBottomPanel ? bottomPanel.height : prv.viewMargins;
        return height;
    }

    QtObject {
        id: prv
        property alias viewer: viewerLoader.item

        property int minimumWidth: viewerModel.effectFamily === EffectFamily.LV2 ? 500 : 250
        property int panelMargins: (viewerModel.effectFamily === EffectFamily.Builtin || viewerModel.viewerComponentType === ViewerComponentType.Generated) ? 16 : 4
        property int viewMargins: (viewerModel.effectFamily === EffectFamily.Builtin || viewerModel.viewerComponentType === ViewerComponentType.Generated) ? 16 : 0
        property int separatorHeight: (viewerModel.effectFamily === EffectFamily.Builtin || viewerModel.viewerComponentType === ViewerComponentType.Generated) ? separator.height + prv.panelMargins : 0
        property bool showTopPanel: viewerModel.effectFamily !== EffectFamily.Builtin || (viewer && viewer.usesPresets)
        property bool showBottomPanel: true

        property bool isApplyAllowed: viewerModel.effectFamily !== EffectFamily.Builtin || (viewer && viewer.isApplyAllowed)
        property bool isPreviewAllowed: !viewer || viewer.isPreviewAllowed !== false
        property bool shouldRollbackOnClose: true

        function closeWindow(accept) {
            if (prv.viewer) {
                prv.viewer.stopPreview();
            }
            // Call later because the preview calls `QCoreApplication::processEvents()`,
            // and we must make sure it doesn't do this after we've closed the dialog, or we'll be getting that Qt exception
            // "Object %p destroyed while one of its QML signal handlers is in progress."
            Qt.callLater(() => {
                prv.shouldRollbackOnClose = !accept;
                accept ? root.accept() : root.reject();
            });
        }
    }

    Connections {
        target: root.window
        function onClosing(event) {
            // Stop preview before closing, for the same reason as in closeWindow()
            if (prv.viewer) {
                prv.viewer.stopPreview();
            }

            if (prv.shouldRollbackOnClose) {
                viewerModel.rollbackSettings();
                presetsBar.presetsBarModel.restoreInitialPresetState();
            }
        }
    }

    Component.onCompleted: {
        viewerModel.load();
        loadViewer();
    }

    onWindowChanged: {
        loadViewer();
    }

    // Listen to UI mode changes from the presets bar menu
    Connections {
        target: presetsBar.presetsBarModel
        function onUseVendorUIChanged() {
            viewerModel.refreshUIMode();
        }
    }

    Connections {
        target: viewerModel
        function onViewerComponentTypeChanged() {
            // For Audio Units, reload the view instead of switching components
            if (viewerModel.viewerComponentType === ViewerComponentType.AudioUnit && prv.viewer) {
                prv.viewer.reload();
            } else {
                loadViewer();
            }
        }
    }

    function loadViewer() {
        switch (viewerModel.viewerComponentType) {
        case ViewerComponentType.AudioUnit:
            viewerLoader.sourceComponent = audioUnitViewerComponent;
            break;
        case ViewerComponentType.Lv2:
            viewerLoader.sourceComponent = lv2ViewerComponent;
            break;
        case ViewerComponentType.Vst:
            viewerLoader.sourceComponent = vstViewerComponent;
            break;
        case ViewerComponentType.Builtin:
            viewerLoader.sourceComponent = builtinViewerComponent;
            break;
        case ViewerComponentType.Generated:
            viewerLoader.sourceComponent = generatedViewerComponent;
            break;
        default:
            viewerLoader.sourceComponent = null;
        }
    }

    DestructiveEffectViewerDialogModel {
        id: viewerModel
        instanceId: root.instanceId
    }

    Component {
        id: audioUnitViewerComponent
        AudioUnitViewer {
            height: implicitHeight

            instanceId: root.instanceId
            topPadding: topPanel.height
            bottomPadding: bbox.implicitHeight + prv.panelMargins * 2
            sidePadding: prv.viewMargins
            minimumWidth: prv.minimumWidth
        }
    }

    Component {
        id: lv2ViewerComponent
        Lv2Viewer {
            instanceId: root.instanceId
            title: root.title

            onVendorUiFailed: {
                Qt.callLater(viewerModel.notifyVendorUiFailed);
            }
        }
    }

    Component {
        id: vstViewerComponent
        VstViewer {
            height: implicitHeight

            instanceId: root.instanceId
            topPadding: topPanel.height
            bottomPadding: bbox.implicitHeight + prv.panelMargins * 2
            sidePadding: prv.viewMargins
            minimumWidth: prv.minimumWidth
        }
    }

    Component {
        id: builtinViewerComponent
        BuiltinEffectViewer {
            instanceId: root.instanceId
            dialogView: root
            usedDestructively: true
        }
    }

    Component {
        id: generatedViewerComponent
        GeneratedEffectViewer {
            instanceId: root.instanceId
            dialogView: root
        }
    }

    Column {
        anchors.fill: parent

        WindowContainer {
            id: topPanelContainer

            width: parent.width
            height: presetsBar.height + prv.separatorHeight + prv.panelMargins * 2

            visible: prv.showTopPanel

            window: Window {
                id: topPanel

                width: topPanelContainer.width
                height: topPanelContainer.height

                color: M3.color.surface

                Row {
                    id: headerBar
                    spacing: 4
                    anchors.fill: parent
                    anchors.margins: prv.panelMargins

                    EffectPresetsBar {
                        id: presetsBar

                        width: parent.width

                        destructiveMode: true
                        navigationPanel: root.navigationPanel
                        navigationOrder: 0

                        enabled: !(prv.viewer && prv.viewer.isPreviewing)
                        parentWindow: root.window
                        instanceId: root.instanceId
                    }
                }

                M3Divider {
                    id: separator

                    anchors.top: headerBar.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    visible: (viewerModel.effectFamily === EffectFamily.Builtin || viewerModel.viewerComponentType === ViewerComponentType.Generated)
                }
            }
        }

        Item {
            id: spacer

            visible: !prv.showTopPanel

            width: parent.width
            height: prv.viewMargins
        }

        Loader {
            id: viewerLoader

            anchors.left: parent.left
            anchors.margins: prv.viewMargins
        }

        WindowContainer {
            id: bottomPanelContainer

            width: parent.width
            height: prv.panelMargins * 2 + bbox.height

            window: Window {
                id: bottomPanel

                width: bottomPanelContainer.width
                height: bottomPanelContainer.height

                color: M3.color.surface

                Item {
                    anchors.fill: parent
                    anchors.margins: prv.panelMargins

                    RowLayout {
                        id: bbox

                        anchors.left: parent.left
                        anchors.right: parent.right

                        spacing: prv.panelMargins

                        property NavigationPanel navigationPanel: NavigationPanel {
                            name: "EffectDialogButtons"
                            direction: NavigationPanel.Horizontal
                            section: root.navigationSection
                            order: (prv.showTopPanel ? 1 : 0) + (prv.viewer && prv.viewer.numNavigationPanels !== undefined ? prv.viewer.numNavigationPanels : (viewerModel.effectFamily === EffectFamily.Builtin ? 2 : 0))
                        }

                        M3Button {
                            id: previewBtn

                            minWidth: 80
                            variant: "tonal"

                            visible: prv.isPreviewAllowed

                            navigation.panel: bbox.navigationPanel
                            navigation.order: 0

                            text: (prv.viewer && prv.viewer.isPreviewing) ?
                            //: Shown on a button that stops effect preview
                            qsTrc("effects", "Stop preview") :
                            //: Shown on a button that starts effect preview
                            qsTrc("effects", "Preview")

                            enabled: prv.isApplyAllowed

                            onClicked: {
                                if (!prv.viewer) {
                                    return;
                                }
                                if (prv.viewer.isPreviewing) {
                                    prv.viewer.stopPreview();
                                } else {
                                    prv.viewer.startPreview();
                                }
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        M3Button {
                            id: cancelBtn

                            minWidth: 80
                            variant: "text"

                            navigation.panel: bbox.navigationPanel
                            navigation.order: previewBtn.navigation.order + 1

                            //: Label of a dialog button
                            text: qsTrc("global", "Cancel")

                            onClicked: {
                                prv.closeWindow(false);
                            }
                        }

                        M3Button {
                            id: okBtn

                            minWidth: 80
                            variant: "filled"

                            navigation.panel: bbox.navigationPanel
                            navigation.order: cancelBtn.navigation.order + 1

                            //: Label of the dialog button that applies the effect
                            text: qsTrc("global", "Apply")
                            enabled: prv.isApplyAllowed

                            onClicked: {
                                presetsBar.presetsBarModel.commitSelectedPreset();
                                prv.closeWindow(true);
                            }
                        }
                    }
                }
            }
        }
    }

    EffectControlsDisablingOverlay {
        x: viewerLoader.x
        y: viewerLoader.y
        width: viewerLoader.width
        height: viewerLoader.height

        visible: prv.viewer && prv.viewer.isPreviewing
        effectFamily: root.effectFamily
    }
}
