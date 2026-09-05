/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

M3IconButton {
    id: root

    property bool isMasterEffect: false
    property bool accentButton: false
    property int size: 24

    width: root.size
    height: root.size

    icon: IconCode.BYPASS
    variant: root.accentButton ? "filled" : "standard"

    //: Tooltip of the effect power button
    toolTipTitle: qsTrc("effects", "Bypass effect")
    accessibleName: root.toolTipTitle
}
