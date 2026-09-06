/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.UiComponents
import Audacity.M3

BaseSection {
    id: root

    title: qsTrc("preferences", "Dim sum surprise")

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "On about one launch in ten, a small card names a random dim sum dish in English and Traditional Chinese, with a photo when one is available offline or online. It never blocks startup and there is no setting to turn it off. School mode suppresses it while it is on.")
    }
}
