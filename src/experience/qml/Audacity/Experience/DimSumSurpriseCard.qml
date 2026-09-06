/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Experience

// A small, non-blocking card naming a random dim sum dish. It appears on
// about one launch in ten, never blocks anything, and auto-dismisses.
// There is no way to turn this off from here or anywhere else.
Item {
    id: root

    property alias model: dimSumModel

    DimSumSurpriseModel {
        id: dimSumModel

        onRevealed: {
            card.opacity = 1
            dismissTimer.restart()
        }
    }

    // The model itself reads the real first-run and School mode state; the
    // caller only decides when it is safe to ask (window up, no dialog or
    // background task active).
    function offer() {
        dimSumModel.offerIfDue()
    }

    M3Card {
        id: card

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16

        visible: opacity > 0
        opacity: 0
        variant: "elevated"
        padding: 12
        width: Math.min(parent ? parent.width - 32 : 320, 320)
        implicitHeight: content.implicitHeight + padding * 2

        accessibleName: qsTrc("experience", "Dim sum surprise: %1").arg(dimSumModel.dishLabel)

        Behavior on opacity {
            enabled: !M3.motion.reducedMotion

            NumberAnimation {
                duration: M3.motion.medium2
                easing: M3.motion.standard
            }
        }

        Column {
            id: content

            width: parent.width
            spacing: 8

            StyledTextLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                font: M3.typography.labelMedium
                color: M3.color.onSurfaceVariant
                text: qsTrc("experience", "A little dim sum surprise")
            }

            StyledTextLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                font: M3.typography.titleMedium
                text: dimSumModel.dishLabel
            }

            Rectangle {
                width: parent.width
                height: 96
                radius: M3.shape.medium
                color: M3.color.surfaceVariant
                visible: !dimSumModel.photoAvailable

                StyledTextLabel {
                    anchors.centerIn: parent
                    font: M3.typography.bodySmall
                    color: M3.color.onSurfaceVariant
                    text: qsTrc("experience", "Photo unavailable offline")
                }

                Accessible.role: Accessible.Graphic
                Accessible.name: qsTrc("experience", "Photo of %1, unavailable offline").arg(dimSumModel.dishLabel)
            }

            Image {
                width: parent.width
                height: 96
                fillMode: Image.PreserveAspectCrop
                visible: dimSumModel.photoAvailable
                source: dimSumModel.photoPath

                Accessible.role: Accessible.Graphic
                Accessible.name: dimSumModel.dishLabel
            }
        }

        Timer {
            id: dismissTimer

            interval: 6000
            onTriggered: card.opacity = 0
        }
    }
}
