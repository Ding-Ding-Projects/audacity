/*
* Audacity: A Digital Audio Editor
*
* RegexBuilderSheet
*
* The regular expression builder anchored beside the search field that owns
* it: a non-modal Material 3 side sheet on a wide window, and a full window
* surface below 900 device independent pixels, where a side sheet would leave
* nothing usable beside it.
*
* Every search field creates its own sheet, so the pattern, the flags, the
* sample and the saved test cases of one field never reach another.
*
* API:
*     opened, pattern, storeName, fieldLabel, open(), close(),
*     patternAccepted(pattern), closed()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion

Item {
    id: root

    property bool opened: false
    property string pattern: ""
    property string storeName: "default"
    property string fieldLabel: ""

    //! Below this width the sheet would leave too little beside it, so the
    //! builder takes the whole surface instead.
    property real narrowWidthThreshold: 900

    signal patternAccepted(string pattern)
    signal closed

    readonly property bool narrow: root.width < root.narrowWidthThreshold

    visible: root.opened

    function open() {
        root.opened = true
        if (root.narrow) {
            fullWindowLoader.active = true
        } else {
            sheet.open()
        }
    }

    function close() {
        root.opened = false
        sheet.close()
        fullWindowLoader.active = false
        root.closed()
    }

    // The docked side sheet. Not modal: the field it belongs to stays usable
    // while the builder is open.
    M3SideSheet {
        id: sheet

        anchors.fill: parent
        visible: !root.narrow

        modal: false
        edge: Qt.RightEdge
        sheetWidth: Math.min(480, root.width * 0.45)
        headline: qsTrc("companion", "Regular expression builder")

        opened: root.opened && !root.narrow

        onClosed: {
            if (root.opened) {
                root.close()
            }
        }

        // The builder is created afresh on every open, so its engine, its
        // flags and its sample never carry over from one field to another even
        // when one host serves more than one field.
        Loader {
            anchors.fill: parent
            active: root.opened && !root.narrow

            sourceComponent: RegexBuilder {
                objectName: "RegexBuilderSheetContent"

                pattern: root.pattern
                storeName: root.storeName
                fieldLabel: root.fieldLabel

                onPatternAccepted: function (accepted) {
                    root.pattern = accepted
                    root.patternAccepted(accepted)
                    root.close()
                }
            }
        }
    }

    // The narrow fallback: the builder fills the surface it was opened from.
    Loader {
        id: fullWindowLoader

        anchors.fill: parent
        active: false
        visible: root.narrow && root.opened

        sourceComponent: M3Surface {
            level: 3

            Item {
                anchors.fill: parent
                anchors.margins: 16

                StyledTextLabel {
                    id: fullWindowHeadline

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: closeButton.left
                    horizontalAlignment: Text.AlignLeft
                    text: qsTrc("companion", "Regular expression builder")
                    font: M3.typography.titleLarge
                    color: M3.color.onSurface
                }

                M3IconButton {
                    id: closeButton

                    anchors.top: parent.top
                    anchors.right: parent.right
                    icon: IconCode.CLOSE_X_ROUNDED
                    accessibleName: qsTrc("global", "Close")
                    onClicked: root.close()
                }

                RegexBuilder {
                    anchors.top: fullWindowHeadline.bottom
                    anchors.topMargin: 16
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    objectName: "RegexBuilderFullWindowContent"

                    pattern: root.pattern
                    storeName: root.storeName
                    fieldLabel: root.fieldLabel

                    onPatternAccepted: function (accepted) {
                        root.pattern = accepted
                        root.patternAccepted(accepted)
                        root.close()
                    }
                }
            }
        }
    }
}
