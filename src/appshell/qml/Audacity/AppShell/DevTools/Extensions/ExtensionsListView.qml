/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15

import Muse.UiComponents
import Muse.Extensions 1.0

import Audacity.M3

Rectangle {
    color: M3.color.surface

    DevExtensionsListModel {
        id: devModel
    }

    StyledListView {
        anchors.fill: parent

        model: devModel.extensionsList()

        delegate: M3ListItem {
            anchors.left: parent ? parent.left : undefined
            anchors.right: parent ? parent.right : undefined
            height: 96

            overline: String(model.index + 1)
            headline: modelData.title
            supportingText: "uri: " + modelData.uri + "\ntype: " + modelData.type

            onClicked: devModel.clicked(modelData.uri)
        }
    }
}
