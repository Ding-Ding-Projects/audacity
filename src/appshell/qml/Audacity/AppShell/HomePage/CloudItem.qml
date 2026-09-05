/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents
import Muse.Cloud

import Audacity.M3

Item {
    id: root

    property string cloudTitle: ""
    property bool userIsAuthorized: false
    property string userName: ""
    property url userAvatarUrl
    property url userCollectionUrl

    property NavigationPanel navigationPanel: NavigationPanel {
        name: root.cloudTitle + "Item"
        direction: NavigationPanel.Both

        onActiveChanged: function (active) {
            if (active) {
                firstButton.navigation.requestActive();
                accessibleInfo.ignored = false;
                accessibleInfo.focused = true;
            } else {
                accessibleInfo.ignored = true;
                accessibleInfo.focused = false;
                firstButton.navigation.accessible.ignored = true;
            }
        }
    }

    signal signInRequested
    signal signOutRequested
    signal createAccountRequested
    signal myProfileRequested

    AccessibleItem {
        id: accessibleInfo
        accessibleParent: root.navigationPanel.accessible
        visualItem: root
        role: MUAccessible.Button
        name: {
            var msg = "";
            if (Boolean(root.userIsAuthorized)) {
                msg = "%1. %2. %3. %4".arg(root.cloudTitle).arg(root.userName).arg(root.userCollectionUrl).arg(firstButton.text);
            } else {
                msg = "%1. %2. %3".arg(root.cloudTitle).arg(qsTrc("cloud", "Not signed in")).arg(firstButton.text);
            }

            return msg;
        }
    }

    Text {
        id: cloudTitleLabel

        anchors.left: parent.left
        anchors.right: parent.right

        text: root.cloudTitle

        font: M3.typography.titleMedium
        color: M3.color.onSurface
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
    }

    Rectangle {
        anchors.top: cloudTitleLabel.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        color: M3.color.surfaceContainerLow

        radius: M3.shape.large
        border.width: 1
        border.color: M3.color.outlineVariant

        Item {
            anchors.fill: parent
            anchors.margins: 24

            AccountAvatar {
                id: avatar

                url: root.userAvatarUrl
                side: 100
            }

            Column {
                anchors.top: parent.top
                anchors.left: avatar.right
                anchors.leftMargin: 24
                anchors.right: parent.right

                spacing: 12

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right

                    text: Boolean(root.userIsAuthorized) ? root.userName : qsTrc("cloud", "Not signed in")

                    font: M3.typography.headlineSmall
                    color: M3.color.onSurface
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right

                    text: Boolean(root.userIsAuthorized) ? root.userCollectionUrl : root.cloudTitle

                    font: M3.typography.bodyMedium
                    color: M3.color.onSurfaceVariant
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                }
            }

            Row {
                spacing: 12

                anchors.left: avatar.right
                anchors.leftMargin: 24
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                M3Button {
                    id: firstButton

                    width: (parent.width - parent.spacing) / 2

                    text: Boolean(root.userIsAuthorized) ? qsTrc("cloud", "My profile") : qsTrc("cloud", "Sign in")
                    variant: "filled"

                    navigation.panel: root.navigationPanel
                    navigation.name: "FirstButton"
                    navigation.order: 1
                    navigation.accessible.ignored: true
                    navigation.onActiveChanged: {
                        if (!navigation.active) {
                            navigation.accessible.ignored = false;
                            accessibleInfo.ignored = true;
                        }
                    }

                    onClicked: {
                        if (Boolean(root.userIsAuthorized)) {
                            root.myProfileRequested();
                        } else {
                            root.signInRequested();
                        }
                    }
                }

                M3Button {
                    id: secondButton

                    width: (parent.width - parent.spacing) / 2

                    text: Boolean(root.userIsAuthorized) ? qsTrc("cloud", "Sign out") : qsTrc("cloud", "Create account")
                    variant: Boolean(root.userIsAuthorized) ? "outlined" : "filled"

                    navigation.panel: root.navigationPanel
                    navigation.name: "SecondButton"
                    navigation.order: 2

                    onClicked: {
                        if (Boolean(root.userIsAuthorized)) {
                            root.signOutRequested();
                        } else {
                            root.createAccountRequested();
                        }
                    }
                }
            }
        }
    }
}
