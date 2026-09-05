/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Audacity.M3
import Audacity.Experience

// The corner stack of notification toasts. It never covers the whole window
// and it never takes the keyboard focus away from the work surface.
Item {
    id: root

    property int maximumVisible: 4

    NotificationListModel {
        id: notificationsModel

        historyMode: false

        Component.onCompleted: notificationsModel.load()
    }

    Column {
        id: stack

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        spacing: 12

        Repeater {
            model: notificationsModel

            delegate: ExperienceToast {
                id: toast

                required property var model
                required property int index

                visible: toast.index < root.maximumVisible

                notificationType: toast.model.notificationType
                title: toast.model.title
                body: toast.model.body
                actionText: toast.model.actionText
                persistent: toast.model.persistent

                opacity: toast.visible ? 1.0 : 0.0

                Behavior on opacity {
                    enabled: !M3.motion.reducedMotion

                    NumberAnimation {
                        duration: M3.motion.short4
                        easing: M3.motion.standard
                    }
                }

                onDismissed: notificationsModel.dismiss(toast.model.notificationId)
                onActionTriggered: notificationsModel.triggerAction(toast.model.notificationId)
            }
        }
    }
}
