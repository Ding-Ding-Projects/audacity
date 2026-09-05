/*
* Audacity: A Digital Audio Editor
*
* M3FilePicker
*
* A Material 3 file or directory picker: an outlined text field carrying the
* current path next to an icon button that opens the platform picker. The
* public API matches the muse FilePicker, including the PickerType values, so
* that call sites can be switched over mechanically.
*
* Replaces: Muse.UiComponents FilePicker.
*
* API:
*     pickerType, path, dialogTitle, filter, dir, buttonText, buttonWidth,
*     showPathField, pathFieldTitle, pathFieldWidth, spacing, navigation,
*     navigationRowOrderStart, navigationColumnOrderStart, pathEdited(newPath)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    enum PickerType {
        File,
        Directory,
        MultipleDirectories,
        Any
    }

    property int pickerType: M3FilePicker.PickerType.File

    property alias path: pathField.currentText

    property alias dialogTitle: filePickerModel.title
    property alias filter: filePickerModel.filter
    property alias dir: filePickerModel.dir

    property string buttonText: qsTrc("ui", "Browse")
    property alias buttonWidth: button.implicitWidth

    // A horizontal orientation gives the browse button a visible label, the
    // vertical default leaves it as an icon only button.
    property int buttonOrientation: Qt.Vertical
    readonly property bool labelledButton: root.buttonOrientation === Qt.Horizontal

    property NavigationPanel navigation: null
    property int navigationRowOrderStart: 0
    property int navigationColumnOrderStart: 0

    property alias showPathField: pathField.visible
    property string pathFieldTitle: qsTrc("ui", "Current path:")
    property alias pathFieldWidth: pathField.implicitWidth

    property alias spacing: row.spacing

    signal pathEdited(var newPath)

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    FilePickerModel {
        id: filePickerModel
    }

    Item {
        id: prv

        readonly property bool isNavigationBoth: root.navigation && root.navigation.direction === NavigationPanel.Both

        states: [
            State {
                when: prv.isNavigationBoth
                PropertyChanges {
                    pathField.navigation.row: root.navigationRowOrderStart
                    pathField.navigation.column: root.navigationColumnOrderStart

                    button.navigation.row: root.navigationRowOrderStart
                    button.navigation.column: root.navigationColumnOrderStart + 1
                }
            },
            State {
                when: !prv.isNavigationBoth
                PropertyChanges {
                    pathField.navigation.order: root.navigationRowOrderStart
                    button.navigation.order: root.navigationRowOrderStart + 1
                }
            }
        ]
    }

    RowLayout {
        id: row

        anchors.fill: parent
        spacing: 12

        M3TextField {
            id: pathField

            Layout.fillWidth: true
            Layout.minimumWidth: pathField.implicitWidth
            Layout.alignment: Qt.AlignVCenter

            implicitWidth: 0

            variant: "outlined"

            navigation.name: "PathFieldBox"
            navigation.panel: root.navigation
            navigation.enabled: root.visible && root.enabled
            navigation.accessible.name: root.pathFieldTitle + " " + pathField.currentText

            onTextEditingFinished: function (newTextValue) {
                root.pathEdited(newTextValue)
            }
        }

        M3Button {
            id: button

            Layout.alignment: Qt.AlignVCenter

            icon: IconCode.OPEN_FILE
            variant: "outlined"
            text: root.labelledButton ? root.buttonText : ""
            minWidth: root.labelledButton ? 96 : 40
            horizontalPadding: root.labelledButton ? 16 : 8

            toolTipTitle: root.buttonText

            navigation.name: "FilePickerButton"
            navigation.panel: root.navigation
            navigation.enabled: root.visible && root.enabled
            navigation.accessible.name: root.pickerType === M3FilePicker.PickerType.File ? qsTrc("ui", "Choose file") : qsTrc("ui", "Choose directory")

            onClicked: {
                switch (root.pickerType) {
                case M3FilePicker.PickerType.File:
                    {
                        var selectedFile = filePickerModel.selectFile()
                        if (Boolean(selectedFile)) {
                            root.pathEdited(selectedFile)
                        }
                        break
                    }
                case M3FilePicker.PickerType.Directory:
                    {
                        var selectedDirectory = filePickerModel.selectDirectory()
                        if (Boolean(selectedDirectory)) {
                            root.pathEdited(selectedDirectory)
                        }
                        break
                    }
                case M3FilePicker.PickerType.MultipleDirectories:
                    {
                        var selectedDirectories = filePickerModel.selectMultipleDirectories(root.path)
                        root.pathEdited(selectedDirectories)
                        break
                    }
                case M3FilePicker.PickerType.Any:
                    {
                        var selectedAny = filePickerModel.selectAny()
                        if (Boolean(selectedAny)) {
                            root.pathEdited(selectedAny)
                        }
                        break
                    }
                }
            }
        }
    }
}
