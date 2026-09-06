pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Muse.Ui
import Muse.UiComponents
import Audacity.M3
import Audacity.Personalize

Column {
    id: root
    spacing: 12
    width: parent ? parent.width : 480
    StyledTextLabel { text: qsTrc("personalize/branding", "Application logo"); font: M3.typography.titleLarge }
    StyledTextLabel { width: parent.width; wrapMode: Text.WordWrap; text: qsTrc("personalize/branding", "This changes the local Personalize preview only. It never changes the executable, installer, updater or release identity.") }
    Rectangle { width: 128; height: 128; radius: M3.shape.medium; color: M3.color.surfaceContainer
        Image { anchors.fill: parent; anchors.margins: 12; fillMode: Image.PreserveAspectFit; source: BrandingModel.previewPath }
    }
    M3FilePicker { id: picker; width: parent.width; buttonOrientation: Qt.Horizontal; buttonText: qsTrc("personalize/branding", "Choose logo"); filter: "Images (*.png *.jpg *.jpeg *.webp *.svg *.ico)"; pathEdited: function(path) { BrandingModel.loadFile(path) } }
    M3Switch { text: qsTrc("personalize/branding", "Crop to fill the preview"); checked: BrandingModel.crop; onToggled: function(value) { BrandingModel.crop = value } }
    M3TextField { width: parent.width; label: qsTrc("personalize/branding", "Background colour"); currentText: BrandingModel.background; onTextEditingFinished: function(text) { BrandingModel.background = text } }
    StyledTextLabel { width: parent.width; wrapMode: Text.WordWrap; text: BrandingModel.status; color: M3.color.onSurfaceVariant }
    M3Button { text: qsTrc("personalize/branding", "Reset logo"); variant: "outlined"; onClicked: BrandingModel.reset() }
}
