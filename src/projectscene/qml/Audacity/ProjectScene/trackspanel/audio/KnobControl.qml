/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

M3Knob {
    id: root

    // A pan knob grows its active arc out of the centre of its range.
    property bool isPanKnob: false

    bidirectional: root.isPanKnob
}
