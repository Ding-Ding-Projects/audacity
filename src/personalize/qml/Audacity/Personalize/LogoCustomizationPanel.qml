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
    function statusText(code) {
        switch (code) {
        case "custom-saved": return qsTrc("personalize/branding", "Custom logo saved locally")
        case "shipped": return qsTrc("personalize/branding", "Using the shipped logo")
        case "unreadable": return qsTrc("personalize/branding", "The selected file could not be read. Choose another local image.")
        case "too-large": return qsTrc("personalize/branding", "This image is too large. Choose a file up to 8 MiB.")
        case "invalid-background": return qsTrc("personalize/branding", "Use a valid background colour, then try again.")
        case "cache-write": return qsTrc("personalize/branding", "The logo cache could not be updated. Your current logo is unchanged.")
        case "cancelled": return qsTrc("personalize/branding", "The logo update was cancelled. Try again when ready.")
        default: return qsTrc("personalize/branding", "This image is unsupported or invalid. Choose a PNG, JPEG, WebP, SVG or ICO image.")
        }
    }
    StyledTextLabel { text: qsTrc("personalize/branding", "Application logo"); font: M3.typography.titleLarge }
    StyledTextLabel { width: parent.width; wrapMode: Text.WordWrap; text: qsTrc("personalize/branding", "This changes local application presentation, including the Material title bar. It never changes the executable, installer, updater or release identity.") }
    Rectangle { width: 128; height: 128; radius: M3.shape.medium; color: M3.color.surfaceContainer
        Image { anchors.fill: parent; anchors.margins: 12; fillMode: Image.PreserveAspectFit; source: BrandingModel.previewPath }
    }
    M3FilePicker { id: picker; width: parent.width; buttonOrientation: Qt.Horizontal; buttonText: qsTrc("personalize/branding", "Choose logo"); filter: "Images (*.png *.jpg *.jpeg *.webp *.svg *.ico)"; pathEdited: function(path) { BrandingModel.loadFile(path) } }
    M3Switch { text: qsTrc("personalize/branding", "Crop to fill the preview"); checked: BrandingModel.crop; enabled: BrandingModel.hasCustomLogo; onToggled: function(value) { BrandingModel.crop = value } }
    M3TextField { width: parent.width; label: qsTrc("personalize/branding", "Background colour"); currentText: BrandingModel.background; enabled: BrandingModel.hasCustomLogo; onTextEditingFinished: function(text) { BrandingModel.background = text } }
    StyledTextLabel { visible: !BrandingModel.hasCustomLogo; width: parent.width; wrapMode: Text.WordWrap; text: qsTrc("personalize/branding", "Choose a local logo before changing crop or background."); color: M3.color.onSurfaceVariant }
    StyledTextLabel { width: parent.width; wrapMode: Text.WordWrap; text: root.statusText(BrandingModel.statusCode); color: M3.color.onSurfaceVariant }
    M3Button { text: qsTrc("personalize/branding", "Reset logo"); variant: "outlined"; onClicked: BrandingModel.reset() }
}
