import QtQuick
import Muse.UiComponents
import Audacity.M3
import Audacity.AppShell

Rectangle {
    id: root
    objectName: "FrontBuildProvenance"
    implicitHeight: labels.implicitHeight + 12
    color: M3.color.surfaceContainer

    AboutModel {
        id: aboutModel
    }

    readonly property string runningVersionLine: {
        const version = aboutModel.buildVersion();
        return version === "" ? qsTrc("appshell", "Version unavailable") : qsTrc("appshell", "Version %1").arg(version);
    }
    readonly property string buildProvenanceLine: {
        const updated = aboutModel.buildUpdatedAtLocal();
        return updated === "" ? qsTrc("appshell", "Build provenance unavailable") : qsTrc("appshell", "Build recorded at %1").arg(updated);
    }

    Column {
        id: labels
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 6
        spacing: 2

        StyledTextLabel {
            objectName: "FrontBuildVersion"
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WrapAnywhere
            accessible.name: root.runningVersionLine
            text: root.runningVersionLine
            font: M3.typography.labelLarge
            color: M3.color.onSurface
        }
        StyledTextLabel {
            objectName: "FrontBuildTimestamp"
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WrapAnywhere
            accessible.name: root.buildProvenanceLine
            text: root.buildProvenanceLine
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
        }
    }
}
