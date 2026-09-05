/*
* Audacity: A Digital Audio Editor
*
* AppearanceEditorPopover
*
* The per element appearance editor. A non-modal side sheet anchored beside
* whichever element opened it, so the element being edited stays visible and
* usable the whole time.
*
* Overrides are read and written through the AppearanceOverrides singleton
* and apply live to any M3 surface that reads them, per element and per
* state (normal, hover, focus, pressed, selected, disabled).
*
* API:
*     openAt(anchorItem, elementId)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Personalize

M3SideSheet {
    id: root

    property string elementId: ""
    property string currentState: "normal"

    edge: Qt.RightEdge
    sheetWidth: 380
    modal: false
    headline: qsTrc("personalize", "Edit appearance")

    readonly property var states: ["normal", "hover", "focus", "pressed", "selected", "disabled"]

    function openAt(anchorItem, id) {
        root.elementId = id
        root.currentState = "normal"
        root.opened = true
        open()
        refresh()
    }

    function refresh() {
        fontField.currentText = AppearanceOverrides.getProperty(root.elementId, "fontFamily", root.currentState) || ""
        sizeField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "fontSize", root.currentState) || "")
        italicSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "italic", root.currentState)
        underlineSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "underline", root.currentState)
        strikethroughSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "strikethrough", root.currentState)
        capsSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "smallCaps", root.currentState)
        radiusField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "radius", root.currentState) || "")
        spacingField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "letterSpacing", root.currentState) || "")
        var storedColor = AppearanceOverrides.getProperty(root.elementId, "color", root.currentState)
        colorPicker.selection = storedColor ? storedColor : "#926BFF"
        propertySearch.currentText = ""
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        StyledTextLabel {
            text: qsTrc("personalize", "Element: %1").arg(root.elementId)
            font: M3.typography.labelMedium
            elide: Text.ElideMiddle
            width: parent.width
        }

        // The property inspector search. Filtering itself is out of scope
        // for this small editor; the field exists so a host regex builder
        // can attach to it, per the search bar contract every settings
        // surface carries.
        M3TextField {
            id: propertySearch
            width: parent.width
            label: qsTrc("personalize", "Find a property")
            placeholder: qsTrc("personalize", "font, colour, radius…")
        }

        Row {
            width: parent.width
            spacing: 4
            Repeater {
                model: root.states
                delegate: M3Chip {
                    required property string modelData
                    text: modelData
                    checked: root.currentState === modelData
                    onClicked: {
                        root.currentState = modelData
                        root.refresh()
                    }
                }
            }
        }

        StyledTextLabel {
            text: qsTrc("personalize", "Typography")
            font: M3.typography.titleSmall
        }

        M3TextField {
            id: fontField
            width: parent.width
            label: qsTrc("personalize", "Font family")
            supportingText: qsTrc("personalize", "Any installed or bundled font")
            onTextEditingFinished: function (text) {
                AppearanceOverrides.setProperty(root.elementId, "fontFamily", text, root.currentState)
            }
        }

        Row {
            width: parent.width
            spacing: 8
            M3TextField {
                id: sizeField
                width: (parent.width - 8) / 2
                label: qsTrc("personalize", "Size")
                onTextEditingFinished: function (text) {
                    AppearanceOverrides.setProperty(root.elementId, "fontSize", parseFloat(text) || 14, root.currentState)
                }
            }
            M3TextField {
                id: spacingField
                width: (parent.width - 8) / 2
                label: qsTrc("personalize", "Letter spacing")
                onTextEditingFinished: function (text) {
                    AppearanceOverrides.setProperty(root.elementId, "letterSpacing", parseFloat(text) || 0, root.currentState)
                }
            }
        }

        M3Switch {
            id: italicSwitch
            text: qsTrc("personalize", "Italic")
            onToggled: function (checked) {
                AppearanceOverrides.setProperty(root.elementId, "italic", checked, root.currentState)
            }
        }
        M3Switch {
            id: underlineSwitch
            text: qsTrc("personalize", "Underline")
            onToggled: function (checked) {
                AppearanceOverrides.setProperty(root.elementId, "underline", checked, root.currentState)
            }
        }
        M3Switch {
            id: strikethroughSwitch
            text: qsTrc("personalize", "Strikethrough")
            onToggled: function (checked) {
                AppearanceOverrides.setProperty(root.elementId, "strikethrough", checked, root.currentState)
            }
        }
        M3Switch {
            id: capsSwitch
            text: qsTrc("personalize", "Small caps")
            onToggled: function (checked) {
                AppearanceOverrides.setProperty(root.elementId, "smallCaps", checked, root.currentState)
            }
        }

        StyledTextLabel {
            text: qsTrc("personalize", "Colour")
            font: M3.typography.titleSmall
        }

        M3ColorPicker {
            id: colorPicker
            width: parent.width
            allowRainbow: true
            rainbowSpeed: AppearanceOverrides.rainbowSpeedLevel
            onAccepted: {
                AppearanceOverrides.setProperty(root.elementId, "color", colorPicker.selection, root.currentState)
            }
        }

        StyledTextLabel {
            text: qsTrc("personalize", "Shape and spacing")
            font: M3.typography.titleSmall
        }

        M3TextField {
            id: radiusField
            width: parent.width
            label: qsTrc("personalize", "Corner radius")
            onTextEditingFinished: function (text) {
                AppearanceOverrides.setProperty(root.elementId, "radius", parseFloat(text) || 0, root.currentState)
            }
        }

        StyledTextLabel {
            text: qsTrc("personalize", "A property with no visible control here is a property this small editor does not yet cover.")
            font: M3.typography.bodySmall
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Row {
            width: parent.width
            spacing: 8
            M3Button {
                text: qsTrc("personalize", "Reset this state")
                variant: "outlined"
                onClicked: {
                    AppearanceOverrides.resetProperty(root.elementId, "fontFamily", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "fontSize", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "italic", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "underline", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "strikethrough", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "smallCaps", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "color", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "radius", root.currentState)
                    AppearanceOverrides.resetProperty(root.elementId, "letterSpacing", root.currentState)
                    root.refresh()
                }
            }
            M3Button {
                text: qsTrc("personalize", "Reset element")
                variant: "text"
                onClicked: {
                    AppearanceOverrides.resetElement(root.elementId)
                    root.refresh()
                }
            }
        }
    }
}
