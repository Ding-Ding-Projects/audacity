/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

ColumnLayout {
    id: root

    required property AboutModel model

    spacing: prv.contentSpacing

    QtObject {
        id: prv

        readonly property int versionTextSpacing: 12
        readonly property int contentSpacing: 16
        readonly property int contentMargin: 16
        readonly property int contentTextMargin: 12
        readonly property int contentTextSpacing: 8

        readonly property string buildUpdatedAtLabel: qsTrc("appshell/about", "Updated at %1 (%2)")

        readonly property string versionSubtitle: qsTrc("appshell/about", "Audacity the free, open source, cross-platform software for recording and editing sounds.")
    }

    Image {
        Layout.fillWidth: true

        source: "qrc:/resources/AboutBanner.png"
        sourceSize.width: root.width

        MouseArea {
            anchors.fill: parent

            property int clickCount: 0

            onClicked: {
                clickCount++;
                if (clickCount % 3 === 0) {
                    root.model.toggleDevMode();
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight

        spacing: prv.versionTextSpacing

        Text {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap

            text: root.model.appVersion()
            font: M3.typography.titleMedium
            color: M3.color.onSurface
        }

        Text {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap

            //! NOTE Build provenance, fixed when the build was configured.
            text: prv.buildUpdatedAtLabel.arg(root.model.buildUpdatedAtLocal()).arg(root.model.buildUpdatedAtUtc())
            visible: root.model.buildUpdatedAtLocal() !== ""

            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
        }

        Text {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap

            text: prv.versionSubtitle
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }
    }

    StyledFlickable {
        id: audacityFlickable

        Layout.fillWidth: true
        Layout.fillHeight: true

        contentHeight: creditsContainer.height

        Rectangle {
            id: creditsContainer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: prv.contentMargin
            anchors.rightMargin: prv.contentMargin

            height: creditsInner.height + prv.contentTextMargin * 2

            color: M3.color.surfaceContainerLow
            radius: M3.shape.medium

            Column {
                id: creditsInner

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.margins: prv.contentTextMargin

                spacing: prv.contentTextSpacing

                Text {
                    width: parent.width
                    text: qsTrc("appshell/about", "Credits")
                    font: M3.typography.titleMedium
                    color: M3.color.onSurface
                    horizontalAlignment: Text.AlignLeft
                }

                Repeater {
                    model: root.model.creditList()

                    Column {
                        width: parent.width
                        spacing: prv.contentTextSpacing

                        Loader {
                            width: parent.width

                            readonly property bool hasSubtitle: Boolean(modelData.subtitle) && modelData.subtitle.length > 0

                            sourceComponent: hasSubtitle ? titleWithSubtitle : titleOnly

                            Component {
                                id: titleOnly

                                Text {
                                    width: parent.width

                                    horizontalAlignment: Text.AlignLeft
                                    wrapMode: Text.WordWrap

                                    text: modelData.title
                                    font: M3.typography.titleSmall
                                    color: M3.color.onSurface
                                }
                            }

                            Component {
                                id: titleWithSubtitle

                                Column {
                                    width: parent.width
                                    spacing: prv.contentTextSpacing

                                    Text {
                                        width: parent.width

                                        horizontalAlignment: Text.AlignLeft
                                        wrapMode: Text.WordWrap

                                        text: modelData.title
                                        font: M3.typography.titleSmall
                                        color: M3.color.onSurface
                                    }

                                    Text {
                                        width: parent.width

                                        horizontalAlignment: Text.AlignLeft
                                        wrapMode: Text.WordWrap

                                        text: modelData.subtitle
                                        font: M3.typography.bodyMedium
                                        color: M3.color.onSurfaceVariant
                                    }
                                }
                            }
                        }

                        Text {
                            width: parent.width

                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.WordWrap
                            textFormat: Text.RichText
                            font: M3.typography.bodyMedium
                            color: M3.color.onSurfaceVariant
                            linkColor: M3.color.primary

                            text: modelData.credits.map(function (c) {
                                let isRaw = c.raw && c.raw.length > 0;
                                if (isRaw) {
                                    return c.raw;
                                }

                                let isUrl = c.url && c.url.length > 0;
                                if (isUrl) {
                                    return '<a href="' + c.url + '">' + c.name + '</a>';
                                }

                                return c.role ? c.name + ", " + c.role : c.name;
                            }).join("<br>")
                        }
                    }
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    color: M3.color.onSurfaceVariant
                    linkColor: M3.color.primary

                    text: {
                        let websiteUrl = root.model.appUrl();
                        return qsTrc("appshell/about", "Audacity website: %1").arg('<a href="' + websiteUrl.url + '">' + websiteUrl.displayName + '</a>');
                    }
                    font: M3.typography.bodyMedium
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        textFormat: Text.RichText
                        text: qsTrc("appshell/about", "<b>Audacity®</b> software is copyright © 1999-%1 Audacity Team.").arg(new Date().getFullYear())
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurfaceVariant
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        textFormat: Text.RichText
                        text: qsTrc("appshell/about", "The name <b>Audacity</b> is a registered trademark.")
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurfaceVariant
                    }
                }
            }
        }
    }
}
