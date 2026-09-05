/*
* Audacity: A Digital Audio Editor
*
* CommandPalette
*
* The command palette surface: a Material 3 dialog-like container holding a
* search bar, a regex toggle chip, a size mode toggle and the result list.
*
* The palette indexes every registered action, every preferences page and
* setting, every appearance control, every open project tab and dock panel and
* every documentation article. Rows with a live setting behind them carry a
* working control inline; every other row teleports to the surface that owns
* it.
*
* API:
*     paletteModel, opened, fullWindow, open(), close(), closed()
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion

FocusScope {
    id: root

    required property var paletteModel

    property bool opened: false
    property bool fullWindow: false

    signal closed

    // The palette is a card in the middle of the window, or the whole window.
    readonly property real cardWidth: Math.min(720, root.width - 96)
    readonly property real cardHeight: Math.min(560, root.height - 96)

    function open() {
        root.opened = true
        root.paletteModel.reload()
        searchBar.forceActiveFocus()
    }

    function close() {
        root.opened = false
        root.closed()
    }

    visible: root.opened

    // The palette owns its arrow, Enter and Escape keys before the search
    // field sees them, so typing and navigating share one field. Tab is left
    // alone so that it reaches the inline controls in the result rows.
    Keys.priority: Keys.BeforeItem
    Keys.onPressed: function (event) {
        switch (event.key) {
        case Qt.Key_Down:
            prv.currentIndex += 1
            prv.clampIndex()
            event.accepted = true
            break
        case Qt.Key_Up:
            prv.currentIndex -= 1
            prv.clampIndex()
            event.accepted = true
            break
        case Qt.Key_PageDown:
            prv.currentIndex += 10
            prv.clampIndex()
            event.accepted = true
            break
        case Qt.Key_PageUp:
            prv.currentIndex -= 10
            prv.clampIndex()
            event.accepted = true
            break
        case Qt.Key_Return:
        case Qt.Key_Enter:
            root.paletteModel.activate(prv.currentIndex)
            event.accepted = true
            break
        case Qt.Key_Escape:
            root.close()
            event.accepted = true
            break
        default:
            break
        }
    }

    QtObject {
        id: prv

        property int currentIndex: 0

        function clampIndex() {
            var count = root.paletteModel.count
            if (count === 0) {
                prv.currentIndex = 0
                return
            }
            if (prv.currentIndex < 0) {
                prv.currentIndex = 0
            }
            if (prv.currentIndex >= count) {
                prv.currentIndex = count - 1
            }
            resultList.positionViewAtIndex(prv.currentIndex, ListView.Contain)
        }
    }

    Connections {
        target: root.paletteModel

        function onCountChanged() {
            prv.currentIndex = 0
        }
    }

    // The scrim. Clicking it dismisses the palette.
    Rectangle {
        anchors.fill: parent
        color: M3.color.scrim
        opacity: root.opened ? 0.32 : 0.0
        visible: !root.fullWindow

        Behavior on opacity {
            NumberAnimation {
                duration: M3.motion.short4
                easing: M3.motion.standard
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    M3Surface {
        id: container

        objectName: "CommandPaletteSurface"

        level: root.fullWindow ? 0 : 3
        shadowVisible: !root.fullWindow
        radius: root.fullWindow ? 0 : M3.shape.extraLarge

        width: root.fullWindow ? root.width : root.cardWidth
        height: root.fullWindow ? root.height : root.cardHeight
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.fullWindow ? 0 : Math.max(48, (root.height - height) / 3)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.fullWindow ? 24 : 16
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                M3SearchBar {
                    id: searchBar

                    objectName: "CommandPaletteSearch"

                    Layout.fillWidth: true

                    placeholder: qsTrc("companion", "Search commands, settings, panels and documentation")
                    showRegexBuilder: true

                    onSearchTextChanged: {
                        root.paletteModel.filter = searchBar.searchText
                    }

                    onRegexBuilderRequested: {
                        regexBuilder.pattern = searchBar.searchText
                        regexBuilder.open()
                    }
                }

                M3Chip {
                    objectName: "CommandPaletteRegexToggle"

                    variant: "filter"
                    text: qsTrc("companion", "Regex")
                    checked: root.paletteModel.useRegex
                    toolTipTitle: qsTrc("companion", "Match the search box as a regular expression")

                    onToggled: function (isChecked) {
                        root.paletteModel.useRegex = isChecked
                    }
                }

                M3IconButton {
                    objectName: "CommandPaletteSizeToggle"

                    icon: root.fullWindow ? IconCode.APP_UNMAXIMIZE : IconCode.APP_MAXIMIZE
                    accessibleName: root.fullWindow ? qsTrc("companion", "Show the palette as a card") : qsTrc("companion", "Show the palette as a full window")

                    onClicked: {
                        root.paletteModel.fullWindow = !root.paletteModel.fullWindow
                    }
                }

                M3IconButton {
                    icon: IconCode.CLOSE_X_ROUNDED
                    accessibleName: qsTrc("global", "Close")
                    onClicked: root.close()
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                visible: !root.paletteModel.filterValid
                text: qsTrc("companion", "That regular expression is not valid: %1").arg(root.paletteModel.filterError)
                font: M3.typography.bodySmall
                color: M3.color.error
            }

            ListView {
                id: resultList

                objectName: "CommandPaletteResults"

                Layout.fillWidth: true
                Layout.fillHeight: true

                clip: true
                model: root.paletteModel
                currentIndex: prv.currentIndex
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: StyledScrollBar {}

                delegate: CommandPaletteRow {
                    id: resultRow

                    // The row's own properties are declared in
                    // CommandPaletteRow.qml, so the model roles are bound to
                    // them here rather than redeclared.
                    required property int index
                    required property var model

                    width: ListView.view.width

                    rowType: resultRow.model.rowType
                    title: resultRow.model.title
                    subtitle: resultRow.model.subtitle
                    section: resultRow.model.section
                    shortcut: resultRow.model.shortcut
                    rowEnabled: resultRow.model.rowEnabled
                    controlType: resultRow.model.controlType
                    settingKey: resultRow.model.settingKey
                    settingValue: resultRow.model.settingValue
                    options: resultRow.model.options
                    optionLabels: resultRow.model.optionLabels
                    minimum: resultRow.model.minimum
                    maximum: resultRow.model.maximum
                    step: resultRow.model.step

                    paletteModel: root.paletteModel
                    row: resultRow.index
                    selected: resultRow.index === prv.currentIndex
                    navigationPanel: paletteNavigationPanel

                    onActivated: {
                        prv.currentIndex = resultRow.index
                        root.paletteModel.activate(resultRow.index)
                    }
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                visible: root.paletteModel.count === 0
                text: qsTrc("companion", "Nothing matches that search.")
                font: M3.typography.bodyMedium
                color: M3.color.onSurfaceVariant
            }

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "%1 results. Arrow keys move, Enter activates, Tab reaches the inline controls, Escape closes.").arg(root.paletteModel.count)
                font: M3.typography.labelSmall
                color: M3.color.onSurfaceVariant
            }
        }
    }

    NavigationPanel {
        id: paletteNavigationPanel

        name: "CommandPalette"
        enabled: root.opened
        direction: NavigationPanel.Both
    }

    RegexBuilderSheet {
        id: regexBuilder

        anchors.fill: parent

        storeName: "command-palette"
        fieldLabel: qsTrc("companion", "Command palette search")

        onPatternAccepted: function (pattern) {
            root.paletteModel.useRegex = true
            searchBar.searchText = pattern
            root.paletteModel.filter = pattern
        }
    }
}
