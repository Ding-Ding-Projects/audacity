/*
* Audacity: A Digital Audio Editor
*
* M3ColorPicker
*
* A Material 3 colour picker: a hue wheel, a two dimensional saturation and
* value field, an alpha slider, a numeric entry field and a format translator
* covering the named list, HEX, HEX8, RGB, RGBA, HSL, HSLA, HSV, HWB, CIELAB,
* LCH, OKLab, OKLCH and CMYK. It reports when a requested colour had to be
* clipped into the sRGB gamut and shows the WCAG contrast ratio against the
* surface the colour will be used on.
*
* The picker can also offer an animated rainbow choice. A rainbow selection is
* stored as the sentinel string "rainbow" and never as a colour value, so that
* callers can persist the intent rather than a frozen frame of the animation.
* Under reduced motion the rainbow settles on a single hue.
*
* API:
*     selection (a colour string or "rainbow"), color, alpha, format,
*     allowRainbow, rainbowSpeed (1 to 5), contrastBackground,
*     selectionChanged, accepted()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

import "internal/M3ColorFormats.js" as Formats

M3Surface {
    id: root

    // Either a colour understood by the format translator or the exact string
    // "rainbow". Never a colour value when the rainbow choice is active.
    property string selection: "#926BFF"

    property bool allowRainbow: true

    // A level from 1 to 5 rather than a duration, so that the stored value
    // stays meaningful if the animation is retuned later.
    property int rainbowSpeed: 3

    // The surface the chosen colour will sit on, used for the contrast readout.
    property color contrastBackground: M3.color.surface

    property NavigationPanel navigationPanel: null

    signal accepted()

    level: 3
    radius: M3.shape.large

    implicitWidth: 340
    implicitHeight: 520

    readonly property bool isRainbow: root.selection === "rainbow"

    // Working values in HSV, which is what the wheel and the field edit.
    property real hue: 265
    property real saturation: 0.6
    property real brightness: 1.0
    property real alpha: 1.0

    property string format: "hex"
    property bool gamutClipped: false

    readonly property var rgb: Formats.hsvToRgb(root.hue, root.saturation, root.brightness)

    readonly property color currentColor: Qt.rgba(root.rgb[0] / 255, root.rgb[1] / 255,
                                                  root.rgb[2] / 255, root.alpha)

    readonly property real contrast: M3.contrastRatio(root.currentColor, root.contrastBackground)

    readonly property string contrastGrade: {
        if (root.contrast >= 7.0) {
            return "AAA"
        }
        if (root.contrast >= 4.5) {
            return "AA"
        }
        if (root.contrast >= 3.0) {
            return "AA large text only"
        }
        return "below AA"
    }

    // 1 is the slowest level and 5 the fastest.
    readonly property int rainbowCycleDuration: [12000, 8000, 5000, 3000, 1500][
        Math.max(0, Math.min(4, root.rainbowSpeed - 1))]

    function applySelection() {
        if (root.selection === "rainbow") {
            return
        }
        var parsed = Formats.parse(root.selection)
        if (!parsed) {
            return
        }
        root.gamutClipped = parsed.clipped
        var hsv = Formats.rgbToHsv(parsed.r, parsed.g, parsed.b)
        root.hue = hsv[0]
        root.saturation = hsv[1]
        root.brightness = hsv[2]
        root.alpha = parsed.alpha
    }

    function commit() {
        if (root.isRainbow) {
            return
        }
        root.selection = Formats.format(root.format, root.rgb[0], root.rgb[1], root.rgb[2], root.alpha)
    }

    Component.onCompleted: root.applySelection()

    onSelectionChanged: root.applySelection()

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Row {
            width: parent.width
            spacing: 16

            // The hue wheel.
            Item {
                id: wheel

                width: 140
                height: 140

                Canvas {
                    id: wheelCanvas

                    anchors.fill: parent
                    renderStrategy: Canvas.Cooperative

                    onPaint: {
                        var context = wheelCanvas.getContext("2d")
                        var centre = wheelCanvas.width / 2
                        var outer = centre
                        var inner = centre - 18
                        context.clearRect(0, 0, wheelCanvas.width, wheelCanvas.height)
                        for (var degree = 0; degree < 360; ++degree) {
                            var start = (degree - 1) * Math.PI / 180
                            var end = (degree + 1) * Math.PI / 180
                            context.beginPath()
                            context.arc(centre, centre, (outer + inner) / 2, start, end)
                            context.strokeStyle = Qt.hsva(degree / 360, 1, 1, 1)
                            context.lineWidth = outer - inner
                            context.stroke()
                        }
                    }
                }

                // The hue marker.
                Rectangle {
                    readonly property real angle: root.hue * Math.PI / 180
                    readonly property real ringRadius: wheel.width / 2 - 9

                    width: 16
                    height: 16
                    radius: 8
                    antialiasing: true
                    color: "transparent"
                    border.width: 3
                    border.color: M3.color.onSurface

                    x: wheel.width / 2 + Math.cos(angle) * ringRadius - width / 2
                    y: wheel.height / 2 + Math.sin(angle) * ringRadius - height / 2
                }

                MouseArea {
                    id: wheelMouse

                    anchors.fill: parent
                    enabled: !root.isRainbow

                    function updateHue(mouse) {
                        var dx = mouse.x - wheel.width / 2
                        var dy = mouse.y - wheel.height / 2
                        var degrees = Math.atan2(dy, dx) * 180 / Math.PI
                        root.hue = (degrees + 360) % 360
                        root.commit()
                    }

                    onPressed: function(mouse) { wheelMouse.updateHue(mouse) }
                    onPositionChanged: function(mouse) {
                        if (wheelMouse.pressed) {
                            wheelMouse.updateHue(mouse)
                        }
                    }
                }
            }

            // The saturation and value field.
            Item {
                id: field

                width: 140
                height: 140

                Rectangle {
                    anchors.fill: parent
                    radius: M3.shape.small
                    antialiasing: true
                    color: Qt.hsva(root.hue / 360, 1, 1, 1)

                    // Saturation runs left to right.
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#FFFFFFFF" }
                            GradientStop { position: 1.0; color: "#00FFFFFF" }
                        }
                    }

                    // Value runs top to bottom.
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00000000" }
                            GradientStop { position: 1.0; color: "#FF000000" }
                        }
                    }
                }

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    antialiasing: true
                    color: "transparent"
                    border.width: 2
                    border.color: M3.color.onSurface

                    x: root.saturation * field.width - width / 2
                    y: (1 - root.brightness) * field.height - height / 2
                }

                MouseArea {
                    id: fieldMouse

                    anchors.fill: parent
                    enabled: !root.isRainbow

                    function updateColor(mouse) {
                        root.saturation = Math.max(0, Math.min(1, mouse.x / field.width))
                        root.brightness = Math.max(0, Math.min(1, 1 - mouse.y / field.height))
                        root.commit()
                    }

                    onPressed: function(mouse) { fieldMouse.updateColor(mouse) }
                    onPositionChanged: function(mouse) {
                        if (fieldMouse.pressed) {
                            fieldMouse.updateColor(mouse)
                        }
                    }
                }
            }
        }

        // Alpha.
        Row {
            width: parent.width
            spacing: 12

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                horizontalAlignment: Text.AlignLeft
                text: "Alpha"
                font: M3.typography.labelMedium
                color: M3.color.onSurfaceVariant
            }

            M3Slider {
                id: alphaSlider

                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 60
                from: 0.0
                to: 1.0
                value: root.alpha
                enabled: !root.isRainbow
                accessibleName: "Alpha"
                valueText: Math.round(root.alpha * 100) + "%"
                navigation.panel: root.navigationPanel

                onMoved: {
                    root.alpha = alphaSlider.value
                    root.commit()
                }
            }
        }

        // Format translator and numeric entry.
        Row {
            width: parent.width
            spacing: 12

            M3Dropdown {
                id: formatDropdown

                width: 120
                label: "Format"
                model: ["named", "hex", "hex8", "rgb", "rgba", "hsl", "hsla",
                        "hsv", "hwb", "lab", "lch", "oklab", "oklch", "cmyk"]
                currentIndex: 1
                navigation.panel: root.navigationPanel

                onActivated: function(index, value) {
                    root.format = String(value)
                    root.commit()
                }
            }

            M3TextField {
                id: entry

                width: parent.width - 132
                label: "Value"
                enabled: !root.isRainbow
                currentText: root.isRainbow
                             ? "rainbow"
                             : Formats.format(root.format, root.rgb[0], root.rgb[1],
                                              root.rgb[2], root.alpha)
                navigation.panel: root.navigationPanel

                onTextEditingFinished: function(text) {
                    var parsed = Formats.parse(text)
                    if (!parsed) {
                        entry.hasError = true
                        return
                    }
                    entry.hasError = false
                    root.gamutClipped = parsed.clipped
                    var hsv = Formats.rgbToHsv(parsed.r, parsed.g, parsed.b)
                    root.hue = hsv[0]
                    root.saturation = hsv[1]
                    root.brightness = hsv[2]
                    root.alpha = parsed.alpha
                    root.commit()
                }
            }
        }

        // Gamut and contrast readout.
        Column {
            width: parent.width
            spacing: 4

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                visible: root.gamutClipped
                text: "This colour lies outside the sRGB gamut and has been clipped to fit."
                wrapMode: Text.WordWrap
                font: M3.typography.bodySmall
                color: M3.color.error
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                text: "Contrast " + root.contrast.toFixed(2) + " to 1, " + root.contrastGrade
                font: M3.typography.bodySmall
                color: M3.color.onSurfaceVariant
            }
        }

        // The animated rainbow choice.
        Row {
            width: parent.width
            visible: root.allowRainbow
            spacing: 12

            M3Chip {
                id: rainbowChip

                anchors.verticalCenter: parent.verticalCenter
                variant: "filter"
                text: "Animated rainbow"
                checked: root.isRainbow
                navigation.panel: root.navigationPanel

                onToggled: function(checked) {
                    if (checked) {
                        root.selection = "rainbow"
                    } else {
                        root.commit()
                    }
                }
            }

            M3Slider {
                id: speedSlider

                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - rainbowChip.width - 12
                visible: root.isRainbow
                from: 1
                to: 5
                stepSize: 1
                value: root.rainbowSpeed
                accessibleName: "Rainbow speed"
                valueText: "Level " + Math.round(speedSlider.value)
                navigation.panel: root.navigationPanel

                onMoved: root.rainbowSpeed = Math.round(speedSlider.value)
            }
        }

        // Preview.
        Rectangle {
            width: parent.width
            height: 48
            radius: M3.shape.small
            antialiasing: true
            border.width: 1
            border.color: M3.color.outlineVariant

            // Under reduced motion the rainbow preview settles on one hue.
            color: root.isRainbow
                   ? Qt.hsva(rainbowAnimator.phase, 0.8, 1.0, 1.0)
                   : root.currentColor

            QtObject {
                id: rainbowAnimator

                property real phase: 0.72
            }

            NumberAnimation {
                target: rainbowAnimator
                property: "phase"
                running: root.isRainbow && !M3.motion.reducedMotion
                loops: Animation.Infinite
                from: 0.0
                to: 1.0
                duration: root.rainbowCycleDuration
            }
        }
    }
}
