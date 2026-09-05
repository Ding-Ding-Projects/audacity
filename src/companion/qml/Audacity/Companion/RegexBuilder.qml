/*
* Audacity: A Digital Audio Editor
*
* RegexBuilder
*
* The regular expression builder workbench. It carries, in one scrolling
* column: the raw pattern and the flags, a guided token catalogue that writes
* into the same pattern, a token by token explanation, a parse tree, a live
* match list with its capture table against a sample, a replacement preview,
* the measured run time, the backtracking risk report, the saved test cases
* with JSON import and export, and the dialect capability matrix.
*
* Every instance owns its own RegexEngine, so two search fields never share
* state.
*
* API:
*     pattern, sampleText, storeName, fieldLabel, patternAccepted(pattern)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion

Item {
    id: root

    property alias pattern: engine.pattern
    property alias sampleText: engine.sampleText
    property string storeName: "default"
    property string fieldLabel: ""

    signal patternAccepted(string pattern)

    implicitWidth: 420
    implicitHeight: 640

    RegexEngine {
        id: engine

        storeName: root.storeName
        sampleText: "The quick brown fox jumps over the lazy dog.\nAudacity 4.0.0 released 2026-09-05.\ntrack-01.wav track-02.wav track-10.wav"
    }

    QtObject {
        id: prv

        property string selectedGroup: ""

        readonly property var groups: {
            var seen = []
            var catalog = engine.tokenCatalog()
            for (var i = 0; i < catalog.length; ++i) {
                if (seen.indexOf(catalog[i].group) === -1) {
                    seen.push(catalog[i].group)
                }
            }
            return seen
        }

        function riskColor(level) {
            if (level >= 3) {
                return M3.color.error
            }
            if (level === 2) {
                return M3.color.tertiary
            }
            if (level === 1) {
                return M3.color.secondary
            }
            return M3.color.onSurfaceVariant
        }
    }

    NavigationPanel {
        id: builderPanel

        name: "RegexBuilder"
        direction: NavigationPanel.Both
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: column.implicitHeight
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: StyledScrollBar {}

        ColumnLayout {
            id: column

            width: parent.width
            spacing: 16

            // ---------------------------------------------------------------
            // The raw pattern, always in sync with the guided construction.
            // ---------------------------------------------------------------
            M3TextField {
                id: patternField

                objectName: "RegexPatternField"

                Layout.fillWidth: true

                label: qsTrc("companion", "Pattern")
                placeholder: qsTrc("companion", "Type a pattern, or build one below")
                currentText: engine.pattern
                hasError: !engine.valid
                errorText: engine.valid ? "" : qsTrc("companion", "%1 at offset %2").arg(engine.errorString).arg(engine.errorOffset)
                supportingText: root.fieldLabel !== "" ? qsTrc("companion", "Applies to: %1").arg(root.fieldLabel) : ""

                navigation.panel: builderPanel
                navigation.order: 1

                onTextEdited: function (text) {
                    engine.pattern = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                M3Button {
                    text: qsTrc("companion", "Use this pattern")
                    variant: "filled"
                    enabled: engine.valid && engine.pattern !== ""
                    navigation.panel: builderPanel
                    navigation.order: 2
                    onClicked: root.patternAccepted(engine.pattern)
                }

                M3Button {
                    text: qsTrc("companion", "Escape as literal")
                    variant: "outlined"
                    navigation.panel: builderPanel
                    navigation.order: 3
                    onClicked: engine.pattern = engine.escapeLiteral(engine.pattern)
                }
            }

            // ---------------------------------------------------------------
            // Flags
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "Flags")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                M3Chip {
                    objectName: "RegexFlagCaseInsensitive"
                    variant: "filter"
                    text: qsTrc("companion", "Ignore case")
                    toolTipTitle: "i"
                    checked: engine.caseInsensitive
                    onToggled: function (isChecked) {
                        engine.caseInsensitive = isChecked
                    }
                }

                M3Chip {
                    variant: "filter"
                    text: qsTrc("companion", "Multiline")
                    toolTipTitle: "m"
                    checked: engine.multiline
                    onToggled: function (isChecked) {
                        engine.multiline = isChecked
                    }
                }

                M3Chip {
                    variant: "filter"
                    text: qsTrc("companion", "Dot matches newline")
                    toolTipTitle: "s"
                    checked: engine.dotAll
                    onToggled: function (isChecked) {
                        engine.dotAll = isChecked
                    }
                }

                M3Chip {
                    variant: "filter"
                    text: qsTrc("companion", "Extended")
                    toolTipTitle: "x"
                    checked: engine.extended
                    onToggled: function (isChecked) {
                        engine.extended = isChecked
                    }
                }

                M3Chip {
                    variant: "filter"
                    text: qsTrc("companion", "Unicode properties")
                    toolTipTitle: "u"
                    checked: engine.unicodeProperties
                    onToggled: function (isChecked) {
                        engine.unicodeProperties = isChecked
                    }
                }
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Guided construction
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "Build")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            Repeater {
                model: prv.groups

                delegate: ColumnLayout {
                    id: groupDelegate

                    required property string modelData

                    Layout.fillWidth: true
                    spacing: 4

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: groupDelegate.modelData
                        font: M3.typography.labelLarge
                        color: M3.color.onSurfaceVariant
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: engine.tokenCatalog().filter(function (entry) {
                                return entry.group === groupDelegate.modelData
                            })

                            delegate: M3Chip {
                                required property var modelData

                                variant: "suggestion"
                                text: modelData.label
                                toolTipTitle: modelData.fragment + ": " + modelData.help

                                onClicked: {
                                    engine.insertFragment(modelData.fragment, -1)
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                M3Button {
                    text: qsTrc("companion", "Wrap in a capture group")
                    variant: "text"
                    onClicked: engine.wrapSelection("capture", "", 0, engine.pattern.length)
                }

                M3Button {
                    text: qsTrc("companion", "Wrap in an atomic group")
                    variant: "text"
                    onClicked: engine.wrapSelection("atomic", "", 0, engine.pattern.length)
                }
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Explanation
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "What this pattern says")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            ColumnLayout {
                objectName: "RegexExplanation"

                Layout.fillWidth: true
                spacing: 2

                Repeater {
                    model: engine.explanation

                    delegate: RowLayout {
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 8

                        StyledTextLabel {
                            Layout.leftMargin: modelData.depth * 16
                            Layout.preferredWidth: 96
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            text: modelData.text
                            font: ui.theme.bodyFont
                            color: M3.color.primary
                        }

                        StyledTextLabel {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.WordWrap
                            text: modelData.description
                            font: M3.typography.bodySmall
                            color: M3.color.onSurfaceVariant
                        }
                    }
                }
            }

            // ---------------------------------------------------------------
            // Parse tree
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "Parse tree")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            StyledTextLabel {
                objectName: "RegexParseTree"

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                font: ui.theme.bodyFont
                color: M3.color.onSurfaceVariant
                text: {
                    function render(node, depth) {
                        var pad = "    ".repeat(depth)
                        var line = pad + node.kind + " " + node.text
                        if (Boolean(node.quantifier) && node.quantifier !== "") {
                            line += "  (" + node.quantifier + ")"
                        }
                        var out = [line]
                        var children = node.children || []
                        for (var i = 0; i < children.length; ++i) {
                            out.push(render(children[i], depth + 1))
                        }
                        return out.join("\n")
                    }
                    var tree = engine.parseTree
                    if (!Boolean(tree) || !Boolean(tree.children)) {
                        return qsTrc("companion", "No pattern yet.")
                    }
                    var lines = []
                    for (var i = 0; i < tree.children.length; ++i) {
                        lines.push(render(tree.children[i], 0))
                    }
                    if (tree.balanced === false) {
                        lines.push(qsTrc("companion", "(the brackets in this pattern are not balanced)"))
                    }
                    return lines.join("\n")
                }
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Sample, matches and captures
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "Sample text")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            M3TextField {
                objectName: "RegexSampleField"

                Layout.fillWidth: true

                label: qsTrc("companion", "Sample")
                currentText: engine.sampleText
                supportingText: engine.truncated ? qsTrc("companion", "The sample was shortened before it was matched.") : ""

                onTextEdited: function (text) {
                    engine.sampleText = text
                }
            }

            StyledTextLabel {
                objectName: "RegexMatchSummary"

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: engine.valid ? qsTrc("companion", "%1 matches in %2 ms").arg(engine.matchCount).arg(engine.lastRunMilliseconds.toFixed(3)) : qsTrc("companion", "The pattern is not valid, so nothing was matched.")
                font: M3.typography.labelLarge
                color: engine.valid ? M3.color.onSurface : M3.color.error
            }

            ColumnLayout {
                objectName: "RegexMatchList"

                Layout.fillWidth: true
                spacing: 4

                Repeater {
                    model: engine.matches

                    delegate: ColumnLayout {
                        id: matchDelegate

                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 2

                        StyledTextLabel {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            text: qsTrc("companion", "Match %1 at %2: %3").arg(matchDelegate.modelData.index + 1).arg(matchDelegate.modelData.start).arg(matchDelegate.modelData.text)
                            font: M3.typography.bodyMedium
                            color: M3.color.onSurface
                        }

                        Repeater {
                            model: matchDelegate.modelData.captures

                            delegate: StyledTextLabel {
                                required property var modelData

                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                horizontalAlignment: Text.AlignLeft
                                elide: Text.ElideRight
                                text: qsTrc("companion", "Group %1 %2: %3").arg(modelData.group).arg(modelData.name !== "" ? "(" + modelData.name + ")" : "").arg(modelData.matched ? modelData.text : qsTrc("companion", "did not participate"))
                                font: M3.typography.bodySmall
                                color: M3.color.onSurfaceVariant
                            }
                        }
                    }
                }
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Replacement
            // ---------------------------------------------------------------
            M3TextField {
                objectName: "RegexReplacementField"

                Layout.fillWidth: true

                label: qsTrc("companion", "Replacement template")
                placeholder: "\\1"
                currentText: engine.replacement
                supportingText: qsTrc("companion", "Use \\1 or \\g<name> to insert a captured group.")

                onTextEdited: function (text) {
                    engine.replacement = text
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                visible: engine.replacement !== ""
                text: engine.replacementPreview
                font: ui.theme.bodyFont
                color: M3.color.onSurfaceVariant
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Backtracking risk
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "Backtracking risk")
                font: M3.typography.titleSmall
                color: prv.riskColor(engine.riskLevel)
            }

            StyledTextLabel {
                objectName: "RegexRiskSummary"

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                visible: engine.risks.length === 0
                text: qsTrc("companion", "Nothing in this pattern looks likely to backtrack badly.")
                font: M3.typography.bodySmall
                color: M3.color.onSurfaceVariant
            }

            Repeater {
                model: engine.risks

                delegate: ColumnLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 2

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: modelData.title + ": " + modelData.fragment
                        font: M3.typography.labelLarge
                        color: M3.color.error
                    }

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        wrapMode: Text.WordWrap
                        text: modelData.detail
                        font: M3.typography.bodySmall
                        color: M3.color.onSurfaceVariant
                    }
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                visible: engine.riskLevel >= 2
                text: qsTrc("companion", "Adversarial input warning: a pattern like this one can be made to take an unbounded " + "amount of time by input chosen to defeat it. Do not run it over text that someone " + "else controls until the risk above is resolved.")
                font: M3.typography.bodySmall
                color: M3.color.error
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Saved test cases
            // ---------------------------------------------------------------
            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("companion", "Saved test cases")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                M3TextField {
                    id: testCaseName

                    Layout.fillWidth: true
                    label: qsTrc("companion", "Name")
                }

                M3Button {
                    text: qsTrc("companion", "Save")
                    variant: "tonal"
                    onClicked: engine.saveTestCase(testCaseName.currentText)
                }
            }

            Repeater {
                model: engine.testCases

                delegate: RowLayout {
                    id: testCaseDelegate

                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 8

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        text: testCaseDelegate.modelData.name + ": " + testCaseDelegate.modelData.pattern
                        font: M3.typography.bodySmall
                        color: M3.color.onSurfaceVariant
                    }

                    M3Button {
                        text: qsTrc("companion", "Load")
                        variant: "text"
                        onClicked: engine.loadTestCase(testCaseDelegate.index)
                    }

                    M3IconButton {
                        icon: IconCode.DELETE_TANK
                        accessibleName: qsTrc("companion", "Remove this test case")
                        onClicked: engine.removeTestCase(testCaseDelegate.index)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                M3Button {
                    text: qsTrc("companion", "Export JSON")
                    variant: "outlined"
                    onClicked: {
                        importExport.currentText = engine.exportJson()
                    }
                }

                M3Button {
                    text: qsTrc("companion", "Import JSON")
                    variant: "outlined"
                    enabled: importExport.currentText !== ""
                    onClicked: {
                        if (!engine.importJson(importExport.currentText)) {
                            importExport.hasError = true
                            importExport.errorText = qsTrc("companion", "That is not a valid workbench document.")
                        } else {
                            importExport.hasError = false
                            importExport.errorText = ""
                        }
                    }
                }
            }

            M3TextField {
                id: importExport

                objectName: "RegexImportExportField"

                Layout.fillWidth: true
                label: qsTrc("companion", "Workbench JSON")
                supportingText: qsTrc("companion", "Saved test cases live in %1").arg(engine.storePath())
            }

            M3Divider {
                Layout.fillWidth: true
            }

            // ---------------------------------------------------------------
            // Dialect
            // ---------------------------------------------------------------
            StyledTextLabel {
                objectName: "RegexDialectLabel"

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: engine.dialect
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            Repeater {
                model: engine.capabilities

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 8

                    StyledIconLabel {
                        iconCode: modelData.supported ? IconCode.TICK_RIGHT_ANGLE : IconCode.CLOSE_X_ROUNDED
                        color: modelData.supported ? M3.color.primary : M3.color.error
                    }

                    StyledTextLabel {
                        Layout.preferredWidth: 200
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        text: modelData.name
                        font: M3.typography.bodySmall
                        color: M3.color.onSurface
                    }

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        wrapMode: Text.WordWrap
                        text: modelData.note
                        font: M3.typography.bodySmall
                        color: M3.color.onSurfaceVariant
                    }
                }
            }
        }
    }
}
