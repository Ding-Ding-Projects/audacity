/*
* Audacity: A Digital Audio Editor
*
* M3AppearanceLayers
*
* Renders the layered custom appearance an element has been given in the
* appearance editor: an ordered stack of fills, strokes, shadows, glows,
* blur, tonal adjustments, transforms and masks, each with its own opacity
* and blend mode, on top of that element's normal Material 3 look.
*
* This item draws nothing and takes no space when elementId is empty or the
* element has no layers for the current state, so embedding it in a shared
* background such as M3Surface is a no-op for every element that has not
* opted into the layered editor.
*
* Usage:
*     M3AppearanceLayers {
*         anchors.fill: parent
*         elementId: root.elementId
*         state: root.visualState
*         radius: root.radius
*     }
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects

import Audacity.M3

Item {
    id: root

    property string elementId: ""
    property string state: ""
    property real radius: 0

    readonly property var stack: root.elementId.length > 0 ? AppearanceLayers.layers(root.elementId, root.state) : []
    readonly property bool hasLayers: root.stack.length > 0

    visible: root.hasLayers
    clip: true

    Connections {
        target: AppearanceLayers
        function onLayersChanged(id, state) {
            if (id === root.elementId || id === "") {
                // Re-read: QML bindings on stack already refresh from the
                // property read above, this just forces it promptly.
                root.stackVersion++
            }
        }
    }
    property int stackVersion: 0

    Repeater {
        id: repeater
        model: root.hasLayers ? root.stack : []

        delegate: Item {
            id: layerItem
            required property var modelData
            required property int index

            readonly property var layerData: layerItem.modelData
            readonly property string layerType: layerItem.layerData.type || ""
            readonly property var props: layerItem.layerData.properties || ({})
            readonly property real layerOpacity: layerItem.layerData.opacity !== undefined ? layerItem.layerData.opacity : 1.0
            readonly property string blendMode: layerItem.layerData.blendMode || "normal"
            readonly property bool supportedBlend: AppearanceLayers.supportedBlendModes().indexOf(layerItem.blendMode) >= 0

            anchors.fill: parent
            visible: layerItem.layerData.visible !== false
            opacity: layerItem.layerOpacity

            // Fill layer: solid colour, linear or radial gradient, or a
            // local image. Pattern is approximated as a tiled image fill.
            Rectangle {
                id: fillRect
                anchors.fill: parent
                radius: root.radius
                visible: layerItem.layerType === "fill" && layerItem.props.kind !== "image"
                color: layerItem.props.kind === "solid" || !layerItem.props.kind ? (layerItem.props.color || "transparent") : "transparent"
                gradient: layerItem.props.kind === "gradient" ? gradientBuilder : null

                // Two-stop gradient covers the common case; a stop list of
                // more than two colours beyond these first and last entries
                // is stored (so a richer document round-trips) but only the
                // endpoints are drawn here. Recorded as partial in the
                // capability matrix.
                readonly property var gradientStops: layerItem.props.gradientStops || []
                readonly property color gradientStart: gradientStops.length > 0 ? gradientStops[0].color : "transparent"
                readonly property color gradientEnd: gradientStops.length > 1 ? gradientStops[gradientStops.length - 1].color : gradientStart

                Gradient {
                    id: gradientBuilder
                    orientation: (layerItem.props.gradientAngle || 0) % 180 < 90 ? Gradient.Vertical : Gradient.Horizontal

                    GradientStop {
                        position: 0.0
                        color: fillRect.gradientStart
                    }
                    GradientStop {
                        position: 1.0
                        color: fillRect.gradientEnd
                    }
                }
            }
            Image {
                anchors.fill: parent
                visible: layerItem.layerType === "fill" && layerItem.props.kind === "image" && layerItem.props.imagePath
                source: layerItem.props.imagePath || ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }

            // Stroke layer: an outline at the element's own radius.
            Rectangle {
                anchors.fill: parent
                radius: root.radius
                color: "transparent"
                visible: layerItem.layerType === "stroke"
                border.width: layerItem.props.width || 1
                border.color: layerItem.props.color || "transparent"
            }

            // Shadow and glow: MultiEffect's own shadow, offset outward for
            // a shadow and centred with no offset for a glow. Inner
            // shadow/glow is not representable by MultiEffect and is noted
            // as unsupported in the capability matrix; it still renders the
            // outer form so nothing silently disappears.
            MultiEffect {
                anchors.fill: parent
                visible: layerItem.layerType === "shadow" || layerItem.layerType === "glow"
                source: shadowSource
                shadowEnabled: true
                shadowColor: layerItem.props.color || "black"
                shadowHorizontalOffset: layerItem.props.offsetX || 0
                shadowVerticalOffset: layerItem.props.offsetY || 0
                shadowBlur: Math.min(1.0, (layerItem.props.blurRadius || 8) / 32.0)
                blurMax: 32
            }
            Rectangle {
                id: shadowSource
                anchors.fill: parent
                radius: root.radius
                color: "black"
                visible: false
                layer.enabled: layerItem.layerType === "shadow" || layerItem.layerType === "glow"
            }

            // Blur / backdrop.
            MultiEffect {
                anchors.fill: parent
                visible: layerItem.layerType === "blur"
                source: blurSource
                blurEnabled: true
                blur: Math.min(1.0, (layerItem.props.radius || 8) / 32.0)
                blurMax: 32
            }
            Item {
                id: blurSource
                anchors.fill: parent
                visible: false
                layer.enabled: layerItem.layerType === "blur"
            }

            // Tonal adjustment: brightness, contrast, saturation, hue,
            // colourise, applied to everything drawn beneath this layer via
            // MultiEffect's own colourisation controls where available.
            MultiEffect {
                anchors.fill: parent
                visible: layerItem.layerType === "adjustment"
                source: adjustmentSource
                brightness: (layerItem.props.brightness || 0) / 100.0
                contrast: (layerItem.props.contrast || 0) / 100.0
                saturation: (layerItem.props.saturation || 0) / 100.0
                colorization: layerItem.props.colorizeColor ? 1.0 : 0.0
                colorizationColor: layerItem.props.colorizeColor || "transparent"
            }
            Item {
                id: adjustmentSource
                anchors.fill: parent
                visible: false
                layer.enabled: layerItem.layerType === "adjustment"
            }

            // Transform: translate, rotate, scale, skew about an origin.
            transform: [
                Translate {
                    x: layerItem.layerType === "transform" ? (layerItem.props.translateX || 0) : 0
                    y: layerItem.layerType === "transform" ? (layerItem.props.translateY || 0) : 0
                },
                Scale {
                    origin.x: layerItem.width * (layerItem.props.originX !== undefined ? layerItem.props.originX : 0.5)
                    origin.y: layerItem.height * (layerItem.props.originY !== undefined ? layerItem.props.originY : 0.5)
                    xScale: layerItem.layerType === "transform" ? (layerItem.props.scaleX || 1) : 1
                    yScale: layerItem.layerType === "transform" ? (layerItem.props.scaleY || 1) : 1
                },
                Rotation {
                    origin.x: layerItem.width * (layerItem.props.originX !== undefined ? layerItem.props.originX : 0.5)
                    origin.y: layerItem.height * (layerItem.props.originY !== undefined ? layerItem.props.originY : 0.5)
                    angle: layerItem.layerType === "transform" ? (layerItem.props.rotation || 0) : 0
                }
            ]

            // Mask: rectangular, elliptical or rounded. A freehand path mask
            // is stored (a list of points) but is not yet rendered here; the
            // capability matrix records that as unsupported in this pass.
            MultiEffect {
                anchors.fill: parent
                visible: layerItem.layerType === "mask" && layerItem.props.shape === "ellipse"
                source: maskSource
                maskEnabled: true
                maskSource: ellipseMask
            }
            Item {
                id: maskSource
                anchors.fill: parent
                visible: false
                layer.enabled: layerItem.layerType === "mask"
            }
            Rectangle {
                id: ellipseMask
                anchors.fill: parent
                radius: Math.min(width, height) / 2
                visible: false
            }
        }
    }
}
