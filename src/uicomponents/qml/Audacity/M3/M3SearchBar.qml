/*
* Audacity: A Digital Audio Editor
*
* M3SearchBar
*
* The Material 3 search bar: a fully rounded surface with a leading search
* icon, a clear button and an optional trailing action. It raises
* regexBuilderRequested so that the application's regular expression builder
* can be attached to any search surface.
*
* Replaces: Muse.UiComponents SearchField.
*
* API:
*     searchText, placeholder, trailingIcon, showRegexBuilder,
*     searchTextChanged, accepted(), regexBuilderRequested(), navigation
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property string searchText: ""
    property string placeholder: "Search"
    property string accessibleName: root.placeholder

    // Shows a trailing button that asks the host to open the regex builder.
    property bool showRegexBuilder: false

    property alias navigation: navCtrl

    signal accepted()
    signal regexBuilderRequested()

    implicitHeight: M3.density.apply(56)
    implicitWidth: 320

    function clear() {
        input.text = ""
        root.searchText = ""
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3SearchBar"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.EditableText
        accessible.name: root.accessibleName
        accessible.visualItem: background

        onActiveChanged: {
            if (navCtrl.active) {
                input.forceActiveFocus()
            }
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: height / 2
        antialiasing: true
        color: M3.color.surfaceContainerHigh

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: M3.color.onSurface
            active: root.enabled
            hovered: hoverArea.containsMouse
            focused: navCtrl.highlight
        }
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight && !input.activeFocus
    }

    StyledIconLabel {
        id: searchIcon

        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        iconCode: IconCode.SEARCH
        color: M3.color.onSurfaceVariant
    }

    TextInput {
        id: input

        anchors.left: searchIcon.right
        anchors.leftMargin: 16
        anchors.right: actions.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height

        verticalAlignment: TextInput.AlignVCenter
        clip: true
        selectByMouse: true
        enabled: root.enabled

        color: M3.color.onSurface
        selectionColor: Qt.rgba(M3.color.primary.r, M3.color.primary.g, M3.color.primary.b, 0.4)
        font: M3.typography.bodyLarge

        text: root.searchText

        onTextChanged: root.searchText = input.text
        onAccepted: root.accepted()
        onActiveFocusChanged: {
            if (input.activeFocus) {
                navCtrl.requestActive()
            }
        }

        StyledTextLabel {
            anchors.fill: parent
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            visible: input.text === ""
            text: root.placeholder
            font: M3.typography.bodyLarge
            color: M3.color.onSurfaceVariant
        }
    }

    Row {
        id: actions

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        M3IconButton {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.searchText !== ""
            icon: IconCode.CLOSE_X_ROUNDED
            accessibleName: "Clear search"
            onClicked: root.clear()
        }

        M3IconButton {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.showRegexBuilder
            icon: IconCode.BRACKET_PARENTHESES_SQUARE
            accessibleName: "Open the regular expression builder"
            toolTipTitle: "Regular expression builder"
            onClicked: root.regexBuilderRequested()
        }
    }

    MouseArea {
        id: hoverArea

        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
    }
}
