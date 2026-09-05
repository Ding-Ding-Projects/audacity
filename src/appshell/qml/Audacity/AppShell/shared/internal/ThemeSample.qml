/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick

import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    property var theme

    signal clicked

    width: 88
    height: 64

    //! NOTE The colours below are the previewed theme's own colours, so they
    //! are data rather than Material 3 roles.
    radius: M3.shape.extraSmall
    color: theme.backgroundPrimaryColor

    RoundedRectangle {
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.leftMargin: 12

        color: root.theme.backgroundSecondaryColor

        border.color: root.theme.strokeColor
        border.width: 1

        bottomRightRadius: borderRect.radius

        Column {
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 20
            anchors.leftMargin: 4
            anchors.rightMargin: 4

            spacing: 4

            Rectangle {
                height: 32
                width: parent.width

                radius: M3.shape.extraSmall
                color: root.theme.backgroundPrimaryColor

                border.color: root.theme.strokeColor
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 7

                    spacing: 4

                    Rectangle {
                        width: parent.width
                        height: 3

                        radius: M3.shape.extraSmall
                        color: root.theme.fontPrimaryColor
                    }

                    Rectangle {
                        width: parent.width
                        height: 3

                        radius: M3.shape.extraSmall
                        color: root.theme.fontPrimaryColor
                    }

                    Rectangle {
                        width: (parent.width * 2) / 3
                        height: 3

                        radius: M3.shape.extraSmall
                        color: root.theme.fontPrimaryColor
                    }
                }
            }

            Row {
                width: parent.width
                height: 12

                spacing: 4

                Rectangle {
                    height: parent.height
                    width: 32

                    radius: M3.shape.extraSmall
                    color: Utils.colorWithAlpha(root.theme.buttonColor, root.theme.buttonOpacityNormal)
                    border.color: root.theme.strokeColor
                    border.width: root.theme.borderWidth
                }

                Rectangle {
                    height: parent.height
                    width: 32

                    radius: M3.shape.extraSmall
                    color: Utils.colorWithAlpha(root.theme.accentColor, root.theme.buttonOpacityNormal)
                    border.color: root.theme.strokeColor
                    border.width: root.theme.borderWidth
                }
            }
        }
    }

    Rectangle {
        id: borderRect
        anchors.fill: parent
        color: "transparent"
        radius: M3.shape.extraSmall
        border.width: mouseArea.containsMouse ? 2 : 1
        border.color: mouseArea.containsMouse ? M3.color.primary : M3.color.outlineVariant

        Behavior on border.width {
            NumberAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
