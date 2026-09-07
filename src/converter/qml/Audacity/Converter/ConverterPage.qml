import QtQuick
import QtQuick.Layouts
import Audacity.M3
import Audacity.Converter
M3Surface {
    id: root
    ConverterPresentationModel { id: converter }
    ColumnLayout { anchors.fill: parent; anchors.margins: 24; spacing: 16
        Text { text: qsTrc("converter", "File converter"); font.pixelSize: 28 }
        Text { text: converter.status; wrapMode: Text.Wrap; Layout.fillWidth: true }
        M3TextField { id: input; placeholderText: qsTrc("converter", "Input file path") }
        M3TextField { id: output; placeholderText: qsTrc("converter", "New output file path") }
        M3TextField { id: format; placeholderText: qsTrc("converter", "Output format, for example JPEG") }
        RowLayout { M3Button { text: qsTrc("converter", "Check capabilities"); onClicked: converter.probe() } M3Button { text: qsTrc("converter", "Convert"); enabled: !converter.busy; onClicked: converter.convert(input.text, output.text, format.text) } M3Button { text: qsTrc("converter", "Cancel"); enabled: converter.busy; onClicked: converter.cancel() } }
        M3LinearProgress { visible: converter.busy; value: converter.progress / 100.0; Layout.fillWidth: true }
    }
}
