/*
* Audacity: A Digital Audio Editor
*
* M3Dropdown
*
* A Material 3 exposed dropdown menu: an outlined text field that is not
* editable, with a trailing chevron and a popup list of choices.
*
* Replaces: Muse.UiComponents StyledDropdown. The API is deliberately the same
* so that call sites can be switched over mechanically.
*
* API:
*     model, currentIndex, currentValue, currentText, textRole, valueRole,
*     label, activated(index, value), navigation
*
* A dropdown whose choices are a hierarchical menu model rather than a flat
* list sets menuModel instead of model. The field anatomy is identical and the
* popup becomes an M3Menu, so grouped choices and submenus keep working.
*
*     menuModel, displayText, handleMenuItem(itemId), opened
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion

FocusScope {
    id: root

    property var model: []
    property int currentIndex: -1

    // Items left after the popup's own filter field is applied. Plain text
    // is the default match; the field's own regular expression builder is
    // reached from the same field.
    property string filterText: ""

    readonly property var filteredModel: {
        if (root.filterText === "") {
            return root.model
        }
        var needle = root.filterText.toLowerCase()
        var result = []
        for (var i = 0; i < root.model.length; ++i) {
            if (root.textOf(root.model[i]).toLowerCase().indexOf(needle) !== -1) {
                result.push(root.model[i])
            }
        }
        return result
    }

    property string textRole: "text"
    property string valueRole: "value"

    property string label: ""
    property string placeholder: ""

    // Hierarchical menu model. When set, the popup is an M3Menu and the flat
    // list is not used.
    property var menuModel: null

    // Text shown in the field. Defaults to the current flat list entry.
    property string displayText: root.currentText

    // Height of the field when the label is not shown. A toolbar sets this to
    // a dense value, a form leaves it at the Material default.
    property real fieldHeight: M3.density.apply(56)

    property NavigationPanel menuNavigationPanel: null

    readonly property bool usesMenuModel: root.menuModel !== null && root.menuModel !== undefined
    readonly property bool opened: root.usesMenuModel ? menu.isOpened : popup.isOpened

    property string accessibleName: root.label

    property alias navigation: navCtrl

    signal activated(int index, var value)
    signal handleMenuItem(string itemId)

    function toggleOpened() {
        if (root.usesMenuModel) {
            if (menu.isOpened) {
                menu.close()
            } else {
                menu.open()
            }
        } else {
            root.toggleOpened()
        }
    }

    implicitHeight: root.fieldHeight
    implicitWidth: 200

    function itemAt(index) {
        if (!root.model || index < 0 || index >= root.model.length) {
            return undefined
        }
        return root.model[index]
    }

    function textOf(item) {
        if (item === undefined) {
            return ""
        }
        if (typeof item === "string") {
            return item
        }
        return item[root.textRole] !== undefined ? String(item[root.textRole]) : ""
    }

    function valueOf(item) {
        if (item === undefined) {
            return undefined
        }
        if (typeof item === "string") {
            return item
        }
        return item[root.valueRole]
    }

    readonly property string currentText: root.textOf(root.itemAt(root.currentIndex))
    readonly property var currentValue: root.valueOf(root.itemAt(root.currentIndex))

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Dropdown"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.ComboBox
        accessible.name: root.accessibleName + " " + root.displayText
        accessible.visualItem: background

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.toggleOpened()
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: M3.shape.extraSmall
        antialiasing: true
        color: "transparent"

        border.width: root.opened ? 2 : 1
        border.color: {
            if (!root.enabled) {
                return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
            }
            return root.opened ? M3.color.primary : M3.color.outline
        }

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: M3.color.onSurface
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight
    }

    StyledTextLabel {
        id: floatingLabel

        anchors.left: parent.left
        anchors.leftMargin: 16
        y: root.displayText !== "" ? 8 : (root.height - height) / 2
        visible: root.label !== ""
        text: root.label
        font: root.displayText !== "" ? M3.typography.bodySmall : M3.typography.bodyLarge
        color: root.opened ? M3.color.primary : M3.color.onSurfaceVariant

        Behavior on y {
            NumberAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }
    }

    StyledTextLabel {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: chevron.left
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.label !== "" ? 8 : 0
        height: root.label !== "" ? root.height - 24 : root.height

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: root.displayText !== "" ? root.displayText : root.placeholder
        font: root.height < 40 ? M3.typography.bodyMedium : M3.typography.bodyLarge
        color: root.enabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        elide: Text.ElideRight
    }

    StyledIconLabel {
        id: chevron

        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        iconCode: root.opened ? IconCode.SMALL_ARROW_UP : IconCode.SMALL_ARROW_DOWN
        color: M3.color.onSurfaceVariant
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            navCtrl.requestActive()
            root.toggleOpened()
        }
    }

    StyledPopupView {
        id: popup

        parent: root
        y: root.height
        x: 0

        cornerRadius: M3.shape.extraSmall
        elevationLevel: 2
        backgroundColor: M3.surfaceAt(2)
        borderColor: M3.color.outlineVariant

        padding: 0
        margins: 0
        verticalMargins: 8

        contentWidth: root.width
        contentHeight: dropdownColumn.implicitHeight

        onOpened: {
            root.filterText = ""
            dropdownSearch.searchText = ""
        }

        onClosed: dropdownRegexBuilder.close()

        Column {
            id: dropdownColumn

            width: popup.contentWidth

            M3SearchBar {
                id: dropdownSearch
                objectName: "M3DropdownSearch"

                width: parent.width - 16
                anchors.horizontalCenter: parent.horizontalCenter
                showRegexBuilder: true
                placeholder: qsTrc("uicomponents", "Filter")

                Keys.onEscapePressed: function (event) {
                    if (dropdownSearch.searchText !== "") {
                        dropdownSearch.clear()
                        root.filterText = ""
                        event.accepted = true
                    } else {
                        popup.close()
                        event.accepted = true
                    }
                }

                onSearchTextChanged: root.filterText = dropdownSearch.searchText
                onRegexBuilderRequested: {
                    dropdownRegexBuilder.pattern = dropdownSearch.searchText
                    dropdownRegexBuilder.open()
                }
            }

            StyledTextLabel {
                width: parent.width - 16
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.filterText !== "" && root.filteredModel.length === 0
                horizontalAlignment: Text.AlignHCenter

                //: Shown in a dropdown's filter field when nothing matches the typed text.
                text: qsTrc("uicomponents", "No matching items")
            }

            ListView {
                id: list

                width: dropdownColumn.width
                height: Math.min(300, list.contentHeight)
                clip: true
                model: root.filteredModel

                delegate: M3ListItem {
                    required property int index
                    required property var modelData

                    width: list.width
                    headline: root.textOf(modelData)
                    selected: root.model[root.currentIndex] === modelData
                    leadingIcon: root.model[root.currentIndex] === modelData ? IconCode.TICK_RIGHT_ANGLE : IconCode.NONE

                    onClicked: {
                        var realIndex = root.model.indexOf(modelData)
                        root.currentIndex = realIndex
                        root.activated(realIndex, root.valueOf(modelData))
                        popup.close()
                    }
                }
            }
        }

        RegexBuilderSheet {
            id: dropdownRegexBuilder

            z: 10
            width: popup.contentWidth
            height: popup.contentHeight

            storeName: root.objectName !== "" ? root.objectName : "M3Dropdown"
            fieldLabel: qsTrc("uicomponents", "Filter")

            onPatternAccepted: function (pattern) {
                dropdownSearch.searchText = pattern
                root.filterText = pattern
            }

            onClosed: dropdownSearch.forceActiveFocus()
        }
    }

    M3Menu {
        id: menu

        parent: root
        y: root.height
        x: 0

        model: root.menuModel !== null && root.menuModel !== undefined ? root.menuModel : []
        navigationPanel: root.menuNavigationPanel

        onHandleMenuItem: function (itemId) {
            root.handleMenuItem(itemId)
        }
    }
}
