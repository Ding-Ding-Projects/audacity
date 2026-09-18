/*
* Audacity: A Digital Audio Editor
*
* M3Button
*
* The Material 3 common button in all five variants: filled, tonal, outlined,
* text and elevated. Carries an optional leading icon, an optional loading
* state, a state layer, a ripple, a focus ring and muse navigation.
*
* Replaces: Muse.UiComponents FlatButton.
*
* API:
*     text, icon, variant, enabled, loading, accentButton, minWidth
*     navigation (NavigationControl), clicked()
*
* buttonId, buttonRole and isLeftSide let the muse ButtonBox lay the button out
* and wire the dialog accept and reject defaults, so a dialog can use Material
* buttons without giving up the platform button order.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property string text: ""
    property int icon: IconCode.NONE

    // Identifies this element to the personalize appearance overrides store.
    // Left empty (the default) this button ignores overrides entirely, so the
    // uicomponents module never has to import or link the personalize
    // module: it only reads a global that personalize registers into this
    // same Audacity.M3 namespace when that module is present.
    property string elementId: ""
    property int appearanceRevision: 0

    function m3Appearance(property, fallback) {
        // Referencing appearanceRevision makes every use below reevaluate
        // when the matching override changes.
        root.appearanceRevision
        if (root.elementId === "" || typeof AppearanceOverrides === "undefined") {
            return fallback
        }
        return AppearanceOverrides.resolve(root.elementId, "", property, fallback)
    }

    // One of "filled", "tonal", "outlined", "text" or "elevated".
    property string variant: "filled"

    // Shows a spinner in place of the label and blocks activation.
    property bool loading: false

    property string toolTipTitle: ""
    property string toolTipDescription: ""
    property string toolTipShortcut: ""

    property string accessibleName: root.text

    property real minWidth: 48
    property real horizontalPadding: root.icon !== IconCode.NONE ? 16 : 24

    property alias navigation: navCtrl
    property alias accessible: navCtrl.accessible
    property alias mouseArea: mouseArea

    // Read by the muse ButtonBox and its model.
    property int buttonId: 0
    property int buttonRole: 0
    property bool isLeftSide: false

    // A button box asks for the accent button by this name. It is the filled
    // variant in Material Design 3 terms.
    property bool accentButton: false

    signal clicked

    readonly property bool interactive: root.enabled && !root.loading

    implicitHeight: M3.density.apply(40)
    implicitWidth: Math.max(root.minWidth, contentRow.implicitWidth + root.horizontalPadding * 2)

    readonly property color defaultContainerColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        }
        switch (root.variant) {
        case "tonal":
            return M3.color.secondaryContainer
        case "elevated":
            return M3.surfaceAt(1)
        case "outlined":
        case "text":
            return "transparent"
        default:
            return M3.color.primary
        }
    }

    readonly property color containerColor: root.m3Appearance("containerColor", root.defaultContainerColor)

    readonly property color defaultContentColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        switch (root.variant) {
        case "tonal":
            return M3.color.onSecondaryContainer
        case "outlined":
        case "text":
        case "elevated":
            return M3.color.primary
        default:
            return M3.color.onPrimary
        }
    }

    readonly property color contentColor: root.m3Appearance("contentColor", root.defaultContentColor)
    readonly property real appearanceOpacity: root.m3Appearance("opacity", 1.0)

    readonly property int elevationLevel: {
        if (root.variant !== "elevated" || !root.enabled) {
            return 0
        }
        return mouseArea.containsPress ? 1 : (mouseArea.containsMouse ? 2 : 1)
    }

    Connections {
        target: typeof AppearanceOverrides !== "undefined" ? AppearanceOverrides : null
        ignoreUnknownSignals: true

        function onElementChanged(elementId) {
            if (elementId === root.elementId) {
                root.appearanceRevision = root.appearanceRevision + 1
            }
        }
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Button"
        enabled: root.interactive && root.visible
        accessible.role: MUAccessible.Button
        accessible.name: root.accessibleName
        accessible.enabled: root.interactive

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: {
            ripple.pulse()
            root.clicked()
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: root.m3Appearance("radius", M3.density.apply(40) / 2)
        color: root.containerColor
        border.width: root.variant === "outlined" ? 1 : 0
        border.color: root.enabled ? M3.color.outline : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        antialiasing: true

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        M3Elevation {
            anchors.fill: parent
            level: root.elevationLevel
            radius: background.radius
        }

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: root.contentColor
            active: root.interactive
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
        spacing: 8
        visible: !root.loading
        opacity: root.appearanceOpacity

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            iconCode: root.icon
            visible: root.icon !== IconCode.NONE
            color: root.contentColor
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            visible: root.text !== ""
            font.family: root.m3Appearance("fontFamily", M3.typography.labelLarge.family)
            font.pixelSize: root.m3Appearance("fontSize", M3.typography.labelLarge.pixelSize)
            font.weight: root.m3Appearance("fontWeight", M3.typography.labelLarge.weight)
            font.italic: root.m3Appearance("italic", M3.typography.labelLarge.italic)
            font.letterSpacing: root.m3Appearance("letterSpacing", M3.typography.labelLarge.letterSpacing)
            color: root.contentColor
        }
    }

    M3CircularProgress {
        anchors.centerIn: parent
        visible: root.loading
        running: root.loading
        indicatorColor: root.contentColor
        implicitSize: 20
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.clicked()
        }
    }

    M3ToolTipHandler {
        target: root
        title: root.toolTipTitle
        description: root.toolTipDescription
        shortcut: root.toolTipShortcut
        hovered: mouseArea.containsMouse
    }
}
