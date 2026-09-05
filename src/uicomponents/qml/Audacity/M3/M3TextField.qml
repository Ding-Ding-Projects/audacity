/*
* Audacity: A Digital Audio Editor
*
* M3TextField
*
* The Material 3 text field in the filled and outlined variants. The label
* floats above the text once the field has content or focus. Supports
* supporting text, an error state, leading and trailing icons, a character
* counter and a password mode with a reveal button.
*
* Replaces: Muse.UiComponents TextInputField.
*
* API:
*     currentText, label, placeholder, supportingText, errorText, hasError,
*     variant ("filled" | "outlined"), leadingIcon, trailingIcon, isPassword,
*     maximumLength, textEdited(text), textEditingFinished(text), navigation
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property string currentText: ""
    property string label: ""
    property string placeholder: ""
    property string supportingText: ""
    property string errorText: ""

    property bool hasError: false
    property bool isPassword: false
    property bool readOnly: false

    property string variant: "outlined"

    property int leadingIcon: IconCode.NONE
    property int trailingIcon: IconCode.NONE

    // Zero means no limit and no counter.
    property int maximumLength: 0

    property string accessibleName: root.label

    property alias navigation: navCtrl
    property alias textInput: input

    signal textEdited(string text)
    signal textEditingFinished(string text)
    signal trailingIconClicked

    readonly property bool filled: root.variant === "filled"
    readonly property bool floating: input.activeFocus || root.currentText !== ""
    readonly property bool showError: root.hasError || root.errorText !== ""
    readonly property string helperText: root.showError && root.errorText !== "" ? root.errorText : root.supportingText

    readonly property color accent: root.showError ? M3.color.error : M3.color.primary
    readonly property color labelColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        if (root.showError) {
            return M3.color.error
        }
        return input.activeFocus ? M3.color.primary : M3.color.onSurfaceVariant
    }

    implicitHeight: box.height + (helper.visible ? helper.height + 4 : 0)
    implicitWidth: 240

    function clear() {
        input.text = ""
        root.currentText = ""
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3TextField"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.EditableText
        accessible.name: root.accessibleName
        accessible.visualItem: box

        onActiveChanged: {
            if (navCtrl.active) {
                input.forceActiveFocus()
            }
        }
    }

    Rectangle {
        id: box

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: M3.density.apply(56)
        antialiasing: true

        color: root.filled ? (root.enabled ? M3.color.surfaceContainerHighest : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, 0.04)) : "transparent"

        topLeftRadius: M3.shape.extraSmall
        topRightRadius: M3.shape.extraSmall
        bottomLeftRadius: root.filled ? 0 : M3.shape.extraSmall
        bottomRightRadius: root.filled ? 0 : M3.shape.extraSmall

        border.width: root.filled ? 0 : (input.activeFocus ? 2 : 1)
        border.color: {
            if (!root.enabled) {
                return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
            }
            if (root.showError) {
                return M3.color.error
            }
            return input.activeFocus ? M3.color.primary : M3.color.outline
        }

        Behavior on border.color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        M3StateLayer {
            anchors.fill: parent
            color: M3.color.onSurface
            active: root.enabled && root.filled
            hovered: hoverArea.containsMouse
        }

        // The filled variant carries an indicator line along its bottom edge.
        Rectangle {
            visible: root.filled
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: input.activeFocus ? 2 : 1
            color: {
                if (!root.enabled) {
                    return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
                }
                return root.showError ? M3.color.error : (input.activeFocus ? M3.color.primary : M3.color.onSurfaceVariant)
            }
        }

        StyledIconLabel {
            id: leading

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: root.leadingIcon !== IconCode.NONE
            iconCode: root.leadingIcon
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            id: floatingLabel

            anchors.left: leading.visible ? leading.right : parent.left
            anchors.leftMargin: 12
            text: root.label
            visible: root.label !== ""
            color: root.labelColor
            font: root.floating ? M3.typography.bodySmall : M3.typography.bodyLarge

            y: root.floating ? (root.filled ? 8 : -floatingLabel.height / 2) : (box.height - floatingLabel.height) / 2

            Behavior on y {
                NumberAnimation {
                    duration: M3.motion.short3
                    easing: M3.motion.standard
                }
            }

            // The outlined variant cuts the outline behind the floating label.
            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                z: -1
                visible: !root.filled && root.floating
                color: M3.color.surface
            }
        }

        TextInput {
            id: input

            anchors.left: leading.visible ? leading.right : parent.left
            anchors.leftMargin: 12
            anchors.right: trailingButton.visible ? trailingButton.left : parent.right
            anchors.rightMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.label !== "" ? 8 : 0
            height: root.label !== "" ? box.height - 24 : box.height

            verticalAlignment: TextInput.AlignVCenter
            clip: true
            readOnly: root.readOnly
            enabled: root.enabled
            selectByMouse: true
            echoMode: root.isPassword && !revealed.checked ? TextInput.Password : TextInput.Normal
            maximumLength: root.maximumLength > 0 ? root.maximumLength : 32767

            color: root.enabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
            selectionColor: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.4)
            selectedTextColor: M3.color.onSurface
            font: M3.typography.bodyLarge

            text: root.currentText

            onTextChanged: {
                if (root.currentText !== input.text) {
                    root.currentText = input.text
                    root.textEdited(input.text)
                }
            }

            onEditingFinished: root.textEditingFinished(input.text)

            onActiveFocusChanged: {
                if (input.activeFocus) {
                    navCtrl.requestActive()
                }
            }

            StyledTextLabel {
                anchors.fill: parent
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                visible: input.text === "" && root.label === ""
                text: root.placeholder
                font: M3.typography.bodyLarge
                color: M3.color.onSurfaceVariant
            }
        }

        M3IconButton {
            id: trailingButton

            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter

            visible: root.isPassword || root.trailingIcon !== IconCode.NONE
            icon: root.isPassword ? (revealed.checked ? IconCode.EYE_CLOSED : IconCode.EYE_OPEN) : root.trailingIcon
            accessibleName: root.isPassword ? "Show password" : ""

            onClicked: {
                if (root.isPassword) {
                    revealed.checked = !revealed.checked
                } else {
                    root.trailingIconClicked()
                }
            }
        }

        QtObject {
            id: revealed
            property bool checked: false
        }

        MouseArea {
            id: hoverArea

            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
        }
    }

    M3FocusRing {
        anchors.fill: box
        shapeRadius: M3.shape.extraSmall
        visible: navCtrl.highlight && !input.activeFocus
    }

    Item {
        id: helper

        anchors.top: box.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        height: 16
        visible: root.helperText !== "" || root.maximumLength > 0

        StyledTextLabel {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignLeft
            text: root.helperText
            font: M3.typography.bodySmall
            color: root.showError ? M3.color.error : M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            visible: root.maximumLength > 0
            text: root.currentText.length + " / " + root.maximumLength
            font: M3.typography.bodySmall
            color: root.showError ? M3.color.error : M3.color.onSurfaceVariant
        }
    }
}
