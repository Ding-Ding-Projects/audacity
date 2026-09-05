/*
* Audacity: A Digital Audio Editor
*
* A preferences section: a Material 3 title over a filled card that holds the
* section content. The public API is unchanged from the original section.
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Column {
    id: root

    default property alias contentData: sectionContent.data

    width: parent.width
    spacing: 8

    //! Reported so that the section keeps its content width when it sits in a
    //! layout that sizes its children by their implicit width.
    implicitWidth: Math.max(titleLabel.implicitWidth, sectionContent.implicitWidth + 2 * card.padding)

    property alias title: titleLabel.text
    property int columnWidth: 208
    property int columnSpacing: 12
    property int rowSpacing: 12

    property int navigationOrderStart: 0
    property int navigationOrderEnd: 0
    property NavigationPanel navigation: NavigationPanel {
        name: root.title
        direction: NavigationPanel.Vertical
        accessible.name: root.title
        enabled: root.enabled && root.visible

        onActiveChanged: function (active) {
            if (active) {
                root.forceActiveFocus();
            }
        }
    }

    StyledTextLabel {
        id: titleLabel

        font: M3.typography.titleSmall
        color: M3.color.onSurfaceVariant
        visible: text !== ""
    }

    M3Card {
        id: card

        width: Math.max(root.width, root.implicitWidth)
        height: sectionContent.height + 2 * card.padding

        variant: "filled"
        padding: 16

        Column {
            id: sectionContent

            width: card.width - 2 * card.padding
            spacing: root.rowSpacing
        }
    }
}
