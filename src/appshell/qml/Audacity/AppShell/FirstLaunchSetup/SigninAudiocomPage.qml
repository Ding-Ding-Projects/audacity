/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents
import Audacity.M3
import Audacity.AppShell

Page {
    id: root

    property bool isCreateAccountMode: false

    title: qsTrc("appshell/gettingstarted", "Connect to your audio.com account")
    titleTopMargin: 16

    QtObject {
        id: prv

        readonly property int columnSpacing: 16
        readonly property int columnSideMargin: 100

        readonly property int socialButtonHeight: 32
        readonly property int socialIconTextSpacing: 8

        readonly property string googleAuthProvider: "google"
        readonly property string googleTextLabel: qsTrc("appshell/gettingstarted", "Continue with Google")
        readonly property string orUseEmailText: qsTrc("appshell/gettingstarted", "Or use email and password")
        readonly property int providerLogoSize: 16

        readonly property int textSeparatorSpacing: 16
        readonly property int textInputTitleSpacing: 8
        readonly property int textInputHeight: 28
        readonly property string emailText: qsTrc("appshell/gettingstarted", "Email")
        readonly property string passwordText: qsTrc("appshell/gettingstarted", "Password")
        readonly property string forgotPasswordLink: qsTrc("appshell/gettingstarted", "<a href=\"%1\">Forgot your password?</a>")

        readonly property string noAccountText: qsTrc("appshell/gettingstarted", "Don’t have an account?")
        readonly property string createAccountLinkText: qsTrc("appshell/gettingstarted", "Create new account")
        readonly property string haveAccountText: qsTrc("appshell/gettingstarted", "Already have an account?")
        readonly property string signInLinkText: qsTrc("appshell/gettingstarted", "Sign in")
        readonly property int textLinkSpacing: 4

        readonly property string formButtonTextLoading: qsTrc("appshell/gettingstarted", "Loading…")
        readonly property string formButtonTextSignIn: qsTrc("appshell/gettingstarted", "Sign in")
        readonly property string formButtonTextCreateAccount: qsTrc("appshell/gettingstarted", "Create account")
        readonly property int formButtonHeight: 28
        readonly property int formButtonExtraSpace: model.showErrorMessage ? 0 : 8

        readonly property string forgotPasswordUrl: "https://audio.com/auth/forgot-password"

        readonly property bool canTrigger: !model.authInProgress && emailInputField.currentText.length > 0 && passwordInputField.currentText.length > 0

        function onTriggered() {
            if (!canTrigger) {
                return;
            }

            root.isCreateAccountMode ? model.signUpWithEmail(emailInputField.currentText, passwordInputField.currentText) : model.signInWithEmail(emailInputField.currentText, passwordInputField.currentText);
        }
    }

    Component.onCompleted: {
        model.init();
    }

    SigninAudiocomPageModel {
        id: model

        onAuthorizedChanged: {
            if (authorized) {
                root.navNextPageRequested();
            }
        }
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: prv.columnSideMargin
        anchors.rightMargin: prv.columnSideMargin

        spacing: prv.columnSpacing

        M3Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            variant: "outlined"
            text: prv.googleTextLabel
            accessibleName: prv.googleTextLabel

            NavigationPanel {
                id: socialButtonsPanel
                name: "SocialButtonsPanel"
                enabled: root.enabled && root.visible
                section: root.navigationSection
                order: root.navigationStartRow + 1
                direction: NavigationPanel.Horizontal
                accessible.name: qsTrc("appshell/gettingstarted", "Social sign-in options")
            }

            navigation.name: "GoogleSignInButton"
            navigation.panel: socialButtonsPanel
            navigation.row: 0
            navigation.column: 0

            onClicked: {
                model.signInWithSocial(prv.googleAuthProvider);
            }
        }

        RowLayout {
            spacing: prv.textSeparatorSpacing

            Layout.fillWidth: true
            Layout.preferredHeight: emailSeparatorText.height

            M3Divider {
                Layout.fillWidth: true
                orientation: Qt.Horizontal
            }

            Text {
                id: emailSeparatorText

                text: prv.orUseEmailText
                horizontalAlignment: Text.AlignHCenter
                font: M3.typography.labelLarge
                color: M3.color.onSurfaceVariant
            }

            M3Divider {
                Layout.fillWidth: true
                orientation: Qt.Horizontal
            }
        }

        Column {
            spacing: prv.textInputTitleSpacing
            Layout.fillWidth: true

            NavigationPanel {
                id: emailFieldPanel
                name: "EmailFieldsPanel"
                enabled: root.enabled && root.visible
                section: root.navigationSection
                direction: NavigationPanel.Vertical
                order: root.navigationStartRow + 2
                accessible.name: qsTrc("appshell/gettingstarted", "Email field")
            }

            Text {
                text: prv.emailText
                font: M3.typography.labelLarge
                color: M3.color.onSurfaceVariant
            }

            M3TextField {
                id: emailInputField

                anchors.left: parent.left
                anchors.right: parent.right

                variant: "outlined"
                accessibleName: prv.emailText

                navigation.name: "EmailInput"
                navigation.panel: emailFieldPanel
                navigation.row: 0
                navigation.column: 0

                Connections {
                    target: emailInputField.textInput

                    function onAccepted() {
                        if (prv.canTrigger) {
                            prv.onTriggered();
                        } else {
                            Qt.callLater(emailInputField.textInput.forceActiveFocus);
                        }
                    }
                }
            }
        }

        Column {
            spacing: prv.textInputTitleSpacing
            Layout.fillWidth: true

            RowLayout {
                width: parent.width

                Text {
                    text: prv.passwordText
                    font: M3.typography.labelLarge
                    color: M3.color.onSurfaceVariant
                }

                Item {
                    Layout.fillWidth: true
                }

                FocusableControl {
                    visible: !root.isCreateAccountMode

                    implicitWidth: forgotPasswordLabel.implicitWidth
                    implicitHeight: forgotPasswordLabel.implicitHeight

                    background.color: "transparent"
                    background.border.width: 0

                    NavigationPanel {
                        id: forgetPasswordPanel
                        name: "ForgotPasswordPanel"
                        enabled: root.enabled && root.visible
                        section: root.navigationSection
                        direction: NavigationPanel.Vertical
                        order: root.navigationStartRow + 4
                        accessible.name: qsTrc("appshell/gettingstarted", "Forgot password")
                    }

                    navigation.name: "ForgotPasswordLink"
                    navigation.panel: forgetPasswordPanel
                    navigation.row: 0
                    navigation.column: 0
                    navigation.accessible.name: qsTrc("appshell/gettingstarted", "Forgot password")

                    onNavigationTriggered: {
                        Qt.openUrlExternally(prv.forgotPasswordUrl);
                    }

                    Text {
                        id: forgotPasswordLabel
                        anchors.fill: parent
                        text: prv.forgotPasswordLink.arg(prv.forgotPasswordUrl)
                        textFormat: Text.RichText
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurfaceVariant
                        linkColor: M3.color.primary

                        onLinkActivated: function (link) {
                            Qt.openUrlExternally(link);
                        }
                    }
                }
            }

            M3TextField {
                id: passwordInputField

                anchors.left: parent.left
                anchors.right: parent.right

                variant: "outlined"
                isPassword: true
                accessibleName: prv.passwordText

                NavigationPanel {
                    id: passwordFieldPanel
                    name: "PasswordFieldsPanel"
                    enabled: root.enabled && root.visible
                    section: root.navigationSection
                    direction: NavigationPanel.Vertical
                    order: root.navigationStartRow + 3
                    accessible.name: qsTrc("appshell/gettingstarted", "Password field")
                }

                navigation.name: "PasswordInput"
                navigation.panel: passwordFieldPanel
                navigation.row: 0
                navigation.column: 0

                Connections {
                    target: passwordInputField.textInput

                    function onAccepted() {
                        if (prv.canTrigger) {
                            prv.onTriggered();
                        } else {
                            Qt.callLater(passwordInputField.textInput.forceActiveFocus);
                        }
                    }
                }
            }
        }

        Column {
            spacing: 8
            Layout.fillWidth: true
            Layout.topMargin: prv.formButtonExtraSpace

            Text {
                anchors.left: parent.left

                visible: model.showErrorMessage
                color: M3.color.error
                font: M3.typography.bodyMedium

                text: model.errorMessage
            }

            M3Button {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 40

                enabled: prv.canTrigger

                NavigationPanel {
                    id: actionsPanel
                    name: "ActionsPanel"
                    enabled: root.enabled && root.visible
                    section: root.navigationSection
                    order: root.navigationStartRow + 5
                    accessible.name: qsTrc("appshell/gettingstarted", "Form action")
                }

                variant: "filled"
                loading: model.authInProgress

                text: {
                    if (root.isCreateAccountMode) {
                        return prv.formButtonTextCreateAccount;
                    }

                    return model.authInProgress ? prv.formButtonTextLoading : prv.formButtonTextSignIn;
                }

                navigation.name: "FormButton"
                navigation.panel: actionsPanel

                onClicked: {
                    prv.onTriggered();
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: prv.textLinkSpacing

            Text {
                text: root.isCreateAccountMode ? prv.haveAccountText : prv.noAccountText
                font: M3.typography.bodyMedium
                color: M3.color.onSurfaceVariant
            }

            FocusableControl {
                implicitWidth: accountLinkLabel.implicitWidth
                implicitHeight: accountLinkLabel.implicitHeight

                background.color: "transparent"
                background.border.width: 0

                NavigationPanel {
                    id: accountLinkPanel
                    name: "AccountLinkPanel"
                    enabled: root.enabled && root.visible
                    section: root.navigationSection
                    direction: NavigationPanel.Vertical
                    order: root.navigationStartRow + 6
                    accessible.name: root.isCreateAccountMode ? qsTrc("appshell/gettingstarted", "Sign in link") : qsTrc("appshell/gettingstarted", "Create account link")
                }

                navigation.name: "AccountLink"
                navigation.panel: accountLinkPanel
                navigation.row: 0
                navigation.column: 0
                navigation.accessible.name: root.isCreateAccountMode ? prv.signInLinkText : prv.createAccountLinkText

                onNavigationTriggered: {
                    root.isCreateAccountMode = !root.isCreateAccountMode;
                }

                Text {
                    id: accountLinkLabel
                    anchors.fill: parent
                    text: root.isCreateAccountMode ? prv.signInLinkText : prv.createAccountLinkText
                    color: M3.color.primary
                    font.family: M3.typography.bodyMedium.family
                    font.pixelSize: M3.typography.bodyMedium.pixelSize
                    font.underline: true

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            root.isCreateAccountMode = !root.isCreateAccountMode;
                        }
                    }
                }
            }
        }
    }
}
