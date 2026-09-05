/*
* Audacity: A Digital Audio Editor
*
* M3ToolBarItem
*
* The Material 3 toolbar control used by every project toolbar. It keeps the
* public interface of Muse.UiComponents StyledToolBarItem, so StyledToolBarView
* can load it as a delegate without any other change, but it draws the Material
* 3 anatomy: a container in the toolbar shape, a state layer, a ripple, a three
* pixel focus ring and content in the Material 3 colour roles.
*
* Replaces: Muse.UiComponents StyledToolBarItem.
*
* API:
*     itemData, hasMenu, isMenuSecondary, navigation, accessibleName
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property var itemData: null

    property bool hasMenu: Boolean(itemData) ? itemData.menuItems.length !== 0 : false
    property bool isMenuSecondary: Boolean(itemData) ? itemData.isMenuSecondary : false

    // Shown next to the icon when the model asks for a titled control.
    readonly property bool showsTitle: Boolean(itemData) && itemData.showTitle
    readonly property string title: Boolean(itemData) ? itemData.title : ""

    property alias navigation: navCtrl
    property alias mouseArea: mouseArea

    property string accessibleName: {
        if (!Boolean(root.itemData)) {
            return ""
        }
        if (root.itemData.checkable) {
            // qsTrc is a global helper, so it cannot be qualified by an id.
            // qmllint disable unqualified
            return root.itemData.title + "  " + (root.itemData.checked ? qsTrc("global", "On") : qsTrc("global", "Off"))
            // qmllint enable unqualified
        }
        return root.itemData.title
    }

    // Transport controls carry their own play and record colours, which are
    // data colours rather than brand colours. A fully transparent value means
    // the control uses the ordinary Material 3 roles.
    property color dataIconColor: "transparent"
    property color dataContainerColor: "transparent"
    property bool dataSelected: false

    readonly property bool selected: root.dataSelected || (Boolean(itemData) && (itemData.checked || menuLoader.isMenuOpened))

    enabled: Boolean(itemData) ? itemData.enabled : false

    implicitHeight: M3.density.apply(32)
    implicitWidth: root.showsTitle ? contentRow.implicitWidth + 24 : implicitHeight

    width: implicitWidth
    height: implicitHeight

    readonly property color containerColor: {
        if (!root.enabled) {
            return "transparent"
        }
        if (root.selected) {
            return root.dataContainerColor.a > 0 ? root.dataContainerColor : M3.color.secondaryContainer
        }
        return "transparent"
    }

    readonly property color contentColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        if (root.dataIconColor.a > 0) {
            return root.dataIconColor
        }
        if (root.selected) {
            return M3.color.onSecondaryContainer
        }
        return M3.color.onSurfaceVariant
    }

    function toggleMenuOpened() {
        menuLoader.toggleOpened(root.itemData.menuItems)
    }

    function activateToolBarItem() {
        Qt.callLater(root.itemData.activate)
    }

    NavigationControl {
        id: navCtrl

        name: Boolean(root.itemData) ? root.itemData.id : "M3ToolBarItem"
        enabled: root.enabled && root.visible

        accessible.role: Boolean(root.itemData) && root.itemData.checkable ? MUAccessible.CheckBox : MUAccessible.Button
        accessible.name: root.accessibleName
        accessible.visualItem: root
        accessible.checked: Boolean(root.itemData) && root.itemData.checked

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: {
            ripple.pulse()
            if (menuLoader.isMenuOpened || root.hasMenu) {
                root.toggleMenuOpened()
            } else {
                root.activateToolBarItem()
            }
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: M3.shape.small
        color: root.containerColor
        antialiasing: true

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: root.contentColor
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }

        M3Ripple {
            id: ripple

            anchors.fill: parent
            color: root.contentColor
        }
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight
    }

    Row {
        id: contentRow

        anchors.centerIn: parent
        spacing: root.showsTitle ? 8 : 0

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            iconCode: Boolean(root.itemData) ? root.itemData.icon : IconCode.NONE
            color: root.contentColor
            visible: iconCode !== IconCode.NONE
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.showsTitle
            text: root.title
            font: M3.typography.labelLarge
            color: root.contentColor
        }
    }

    // A control that opens a menu shows the Material 3 trailing marker.
    Rectangle {
        visible: root.isMenuSecondary
        width: 4
        height: 4
        radius: M3.shape.full
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 3
        color: root.contentColor
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        acceptedButtons: root.hasMenu && root.isMenuSecondary ? Qt.LeftButton | Qt.RightButton : Qt.LeftButton

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: function (mouse) {
            navCtrl.requestActive()

            if (menuLoader.isMenuOpened || (root.hasMenu && (!root.isMenuSecondary || mouse.button === Qt.RightButton))) {
                root.toggleMenuOpened()
                return
            }

            if (mouse.button === Qt.LeftButton) {
                root.activateToolBarItem()
            }
        }

        onPressAndHold: function (event) {
            if (menuLoader.isMenuOpened || !root.hasMenu) {
                event.accepted = false
                return
            }

            root.toggleMenuOpened()
        }
    }

    StyledMenuLoader {
        id: menuLoader

        onHandleMenuItem: function (itemId) {
            root.itemData.handleMenuItem(itemId)
        }
    }

    // The muse tooltip provider is a global context property, so these calls
    // cannot be qualified by an id.
    // qmllint disable unqualified
    onEnabledChanged: {
        if (!root.enabled) {
            ui.tooltip.hide(root, true)
        }
    }

    Connections {
        target: mouseArea

        function onContainsMouseChanged() {
            var title = Boolean(root.itemData) ? root.itemData.title : ""
            if (title === "") {
                return
            }
            if (mouseArea.containsMouse) {
                ui.tooltip.show(root, title, Boolean(root.itemData) ? root.itemData.description : "", Boolean(root.itemData) ? root.itemData.shortcuts : "")
            } else {
                ui.tooltip.hide(root)
            }
        }
    }

    Component.onDestruction: {
        ui.tooltip.hide(root, true)
    }
    // qmllint enable unqualified
}
