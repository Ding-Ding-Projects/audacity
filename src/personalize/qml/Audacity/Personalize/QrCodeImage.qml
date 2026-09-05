/*
* Audacity: A Digital Audio Editor
*
* QrCodeImage
*
* Renders a QR code from local text with no image codec and no network
* request anywhere in the path: QrCodeModel builds the module matrix in C++
* and this Canvas paints it directly.
*
* API:
*     text, ok (read only), accessibleDescription
*/
import QtQuick

import Audacity.Personalize

Item {
    id: root

    property string text: ""
    readonly property bool ok: model.ok
    property string accessibleDescription: qsTrc("personalize", "QR code for pairing an authenticator")

    Accessible.role: Accessible.Graphic
    Accessible.name: root.accessibleDescription

    QrCodeModel {
        id: model
        text: root.text
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Immediate

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = "#ffffff"
            ctx.fillRect(0, 0, width, height)

            if (!model.ok || model.size <= 0) {
                return
            }

            var quiet = 4
            var totalModules = model.size + quiet * 2
            var scale = Math.max(1, Math.floor(Math.min(width, height) / totalModules))
            var offset = (Math.min(width, height) - scale * totalModules) / 2

            ctx.fillStyle = "#000000"
            for (var y = 0; y < model.size; ++y) {
                for (var x = 0; x < model.size; ++x) {
                    if (model.moduleAt(x, y)) {
                        ctx.fillRect(offset + (x + quiet) * scale, offset + (y + quiet) * scale, scale, scale)
                    }
                }
            }
        }
    }

    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Connections {
        target: model
        function onTextChanged() {
            canvas.requestPaint()
        }
    }
}
