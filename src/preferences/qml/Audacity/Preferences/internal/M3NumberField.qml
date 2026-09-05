/*
* Audacity: A Digital Audio Editor
*
* M3NumberField
*
* A Material 3 numeric entry built from M3TextField plus a decrement and an
* increment icon button. It keeps the public API of the muse
* IncrementalPropertyControl so that call sites can be swapped mechanically.
*
* Replaces: Muse.UiComponents IncrementalPropertyControl.
*
* API:
*     currentValue, minValue, maxValue, step, decimals, measureUnitsSymbol,
*     wrap, hint, readOnly, navigation, valueEdited(newValue),
*     valueEditingFinished(newValue)
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property real currentValue: 0.0
    property real minValue: -999
    property real maxValue: 999
    property real step: 1
    property int decimals: 0
    property string measureUnitsSymbol: ""
    property bool wrap: false
    property bool readOnly: false

    property string label: ""
    property string hint: ""
    property string accessibleName: root.label !== "" ? root.label : root.hint

    property alias field: field
    property alias navigation: field.navigation

    readonly property bool canDecrease: root.wrap || root.currentValue > root.minValue
    readonly property bool canIncrease: root.wrap || root.currentValue < root.maxValue

    signal valueEdited(var newValue)
    signal valueEditingFinished(var newValue)

    implicitHeight: field.implicitHeight
    implicitWidth: 120

    //! The stepper is a narrow vertical pair, so the value keeps most of the
    //! width even in the compact effect and export layouts.
    readonly property real stepperSize: 24

    function formatted(value) {
        var text = Number(value).toFixed(root.decimals);
        return root.measureUnitsSymbol !== "" ? text + " " + root.measureUnitsSymbol : text;
    }

    function clampValue(value) {
        if (root.wrap) {
            if (value > root.maxValue) {
                return root.minValue;
            }
            if (value < root.minValue) {
                return root.maxValue;
            }
            return value;
        }
        return Math.max(root.minValue, Math.min(root.maxValue, value));
    }

    function commit(value) {
        var next = root.clampValue(value);
        root.currentValue = next;
        root.syncText();
        root.valueEdited(next);
        root.valueEditingFinished(next);
    }

    //! The field owns its own text once the user types into it, so the
    //! displayed value is pushed rather than bound.
    function syncText() {
        var text = root.formatted(root.currentValue);
        if (field.currentText !== text) {
            field.currentText = text;
        }
    }

    onCurrentValueChanged: root.syncText()

    Component.onCompleted: root.syncText()

    function increment() {
        root.commit(root.currentValue + root.step);
    }

    function decrement() {
        root.commit(root.currentValue - root.step);
    }

    function forceActiveFocus() {
        field.forceActiveFocus();
    }

    M3TextField {
        id: field

        anchors.left: parent.left
        anchors.right: stepper.left
        anchors.rightMargin: 2
        anchors.verticalCenter: parent.verticalCenter

        variant: "outlined"
        label: root.label
        placeholder: root.hint
        readOnly: root.readOnly
        accessibleName: root.accessibleName + " " + root.formatted(root.currentValue)

        textInput.inputMethodHints: Qt.ImhFormattedNumbersOnly

        onTextEditingFinished: function (text) {
            var parsed = parseFloat(String(text).replace(",", "."));
            var next = isNaN(parsed) ? root.currentValue : root.clampValue(parsed);
            var changed = next !== root.currentValue;
            root.currentValue = next;
            root.syncText();
            if (changed) {
                root.valueEdited(next);
            }
            root.valueEditingFinished(next);
        }
    }

    Column {
        id: stepper

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        spacing: 0

        M3IconButton {
            width: root.stepperSize
            height: root.stepperSize

            icon: IconCode.SMALL_ARROW_UP
            variant: "standard"
            enabled: root.enabled && !root.readOnly && root.canIncrease
            accessibleName: qsTrc("global", "Increase") + " " + root.accessibleName

            onClicked: root.increment()
        }

        M3IconButton {
            width: root.stepperSize
            height: root.stepperSize

            icon: IconCode.SMALL_ARROW_DOWN
            variant: "standard"
            enabled: root.enabled && !root.readOnly && root.canDecrease
            accessibleName: qsTrc("global", "Decrease") + " " + root.accessibleName

            onClicked: root.decrement()
        }
    }
}
