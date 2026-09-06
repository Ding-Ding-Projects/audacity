/*
* Audacity: A Digital Audio Editor
*
* AppearanceEditorPopover
*
* The per element appearance editor. A resizable, draggable, non-modal side
* sheet anchored beside whichever element opened it, so the element being
* edited stays visible and usable the whole time.
*
* Flat overrides (typography, colour, radius) are read and written through
* the AppearanceOverrides singleton, exactly as before this file grew a
* second, layered workspace. The layered workspace (fill, stroke, shadow,
* glow, blur, tonal adjustment, transform, mask) is read and written through
* the AppearanceLayers singleton and rendered live in the preview by the
* same M3AppearanceLayers item every M3 component uses. Both apply per
* element and per state (normal, hover, focus, pressed, selected, disabled,
* dragged, error, loading, success, warning); a state with no override of
* its own falls back to the normal one.
*
* API:
*     openAt(anchorItem, elementId)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion
import Audacity.Personalize

M3SideSheet {
    id: root

    property string elementId: ""
    property string currentState: "normal"
    property string currentTab: "typography"
    property string selectedLayerId: ""
    property bool beforeAfter: false
    property real previewZoom: 1.0

    // Small in-memory undo stack for this session. Every structural layer
    // change also records into AppearanceLayers' own persistence and into
    // MutationHistory below, so this stack is a convenience on top of an
    // already-durable record rather than the only copy of anything.
    property var undoStack: []
    property var redoStack: []

    edge: Qt.RightEdge
    sheetWidth: 460
    resizable: true
    draggable: true
    modal: false
    headline: qsTrc("personalize", "Edit appearance")

    readonly property var states: ["normal", "hover", "focus", "pressed", "selected", "disabled", "dragged", "error", "loading", "success", "warning"]
    readonly property var tabs: ["typography", "layers", "fill", "stroke", "effects", "adjustments", "transform", "preview"]

    readonly property bool stateOwnsLayerStack: root.currentState === "normal" || AppearanceLayers.hasOwnState(root.elementId, root.currentState)

    function matchesProperty(text) {
        var query = propertySearch.searchText.trim();
        if (query.length === 0) {
            return true;
        }
        try {
            return new RegExp(query, "i").test(text);
        } catch (error) {
            return text.toLowerCase().indexOf(query.toLowerCase()) !== -1;
        }
    }

    function openAt(anchorItem, id) {
        root.elementId = id;
        root.currentState = "normal";
        root.currentTab = "typography";
        root.selectedLayerId = "";
        root.undoStack = [];
        root.redoStack = [];
        root.opened = true;
        open();
        refresh();
    }

    function refresh() {
        fontField.currentText = AppearanceOverrides.getProperty(root.elementId, "fontFamily", root.currentState) || "";
        sizeField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "fontSize", root.currentState) || "");
        italicSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "italic", root.currentState);
        underlineSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "underline", root.currentState);
        strikethroughSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "strikethrough", root.currentState);
        doubleStrikethroughSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "doubleStrikethrough", root.currentState);
        overlineSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "overline", root.currentState);
        capsSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "smallCaps", root.currentState);
        superscriptSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "superscript", root.currentState);
        subscriptSwitch.checked = !!AppearanceOverrides.getProperty(root.elementId, "subscript", root.currentState);
        radiusField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "radius", root.currentState) || "");
        spacingField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "letterSpacing", root.currentState) || "");
        wordSpacingField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "wordSpacing", root.currentState) || "");
        lineHeightField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "lineHeight", root.currentState) || "");
        baselineField.currentText = String(AppearanceOverrides.getProperty(root.elementId, "baselineOffset", root.currentState) || "");
        var storedColor = AppearanceOverrides.getProperty(root.elementId, "color", root.currentState);
        colorPicker.selection = storedColor ? storedColor : "#926BFF";
        propertySearch.searchText = "";
        layerList.model = AppearanceLayers.layers(root.elementId, root.currentState);
        if (root.selectedLayerId === "" && layerList.model.length > 0) {
            root.selectedLayerId = layerList.model[0].id;
        }
    }

    function selectedLayer() {
        var stack = AppearanceLayers.layers(root.elementId, root.currentState);
        for (var i = 0; i < stack.length; i++) {
            if (stack[i].id === root.selectedLayerId) {
                return stack[i];
            }
        }
        return null;
    }

    function pushUndo(label) {
        var snapshot = AppearanceLayers.exportElement(root.elementId);
        root.undoStack.push({
            label: label,
            json: snapshot
        });
        root.redoStack = [];
        MutationHistory.record("appearance-layer-edit", root.elementId + ": " + label);
    }

    function undo() {
        if (root.undoStack.length === 0) {
            return;
        }
        var entry = root.undoStack.pop();
        root.redoStack.push({
            label: entry.label,
            json: AppearanceLayers.exportElement(root.elementId)
        });
        AppearanceLayers.importElement(root.elementId, entry.json);
        MutationHistory.record("appearance-layer-undo", root.elementId + ": " + entry.label);
        root.refresh();
    }

    function redo() {
        if (root.redoStack.length === 0) {
            return;
        }
        var entry = root.redoStack.pop();
        root.undoStack.push({
            label: entry.label,
            json: AppearanceLayers.exportElement(root.elementId)
        });
        AppearanceLayers.importElement(root.elementId, entry.json);
        MutationHistory.record("appearance-layer-redo", root.elementId + ": " + entry.label);
        root.refresh();
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.implicitHeight + 32
        clip: true

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 12

            StyledTextLabel {
                text: qsTrc("personalize", "Element: %1").arg(root.elementId)
                font: M3.typography.labelMedium
                elide: Text.ElideMiddle
                width: parent.width
            }

            M3SearchBar {
                id: propertySearch
                width: parent.width
                placeholder: qsTrc("personalize", "Find a property")
                showRegexBuilder: true
                accessibleName: qsTrc("personalize", "Find appearance properties")
                onRegexBuilderRequested: {
                    propertyRegexBuilder.pattern = propertySearch.searchText;
                    propertyRegexBuilder.open();
                }
            }

            Row {
                width: parent.width
                spacing: 4
                M3Button {
                    text: qsTrc("personalize", "Undo")
                    variant: "text"
                    enabled: root.undoStack.length > 0
                    onClicked: root.undo()
                }
                M3Button {
                    text: qsTrc("personalize", "Redo")
                    variant: "text"
                    enabled: root.redoStack.length > 0
                    onClicked: root.redo()
                }
            }

            Flow {
                width: parent.width
                spacing: 4
                Repeater {
                    model: root.states
                    delegate: M3Chip {
                        required property string modelData
                        text: modelData
                        checked: root.currentState === modelData
                        onClicked: {
                            root.currentState = modelData;
                            root.selectedLayerId = "";
                            root.refresh();
                        }
                    }
                }
            }

            StyledTextLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                text: root.stateOwnsLayerStack ? qsTrc("personalize", "This state owns a layer stack. Its layers are applied by M3 surfaces that use this element identifier.") : qsTrc("personalize", "This state inherits the normal layer stack. Editing a layer here creates a state-specific stack and applies it on M3 surfaces that use this element identifier.")
                font: M3.typography.bodySmall
            }

            M3Tabs {
                width: parent.width
                model: root.tabs
                currentIndex: root.tabs.indexOf(root.currentTab)
                onActivated: function (index) {
                    root.currentTab = root.tabs[index];
                }
            }

            // --- Typography ---------------------------------------------
            Column {
                width: parent.width
                spacing: 8
                visible: root.currentTab === "typography" && root.matchesProperty("typography font family size letter word line baseline italic underline strikethrough overline caps superscript subscript colour color corner radius spacing")

                M3TextField {
                    id: fontField
                    width: parent.width
                    label: qsTrc("personalize", "Font family")
                    supportingText: qsTrc("personalize", "Any installed or bundled font")
                    onTextEditingFinished: function (text) {
                        AppearanceOverrides.setProperty(root.elementId, "fontFamily", text, root.currentState);
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8
                    M3TextField {
                        id: sizeField
                        width: (parent.width - 8) / 2
                        label: qsTrc("personalize", "Size")
                        onTextEditingFinished: function (text) {
                            AppearanceOverrides.setProperty(root.elementId, "fontSize", parseFloat(text) || 14, root.currentState);
                        }
                    }
                    M3TextField {
                        id: spacingField
                        width: (parent.width - 8) / 2
                        label: qsTrc("personalize", "Letter spacing")
                        onTextEditingFinished: function (text) {
                            AppearanceOverrides.setProperty(root.elementId, "letterSpacing", parseFloat(text) || 0, root.currentState);
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8
                    M3TextField {
                        id: wordSpacingField
                        width: (parent.width - 8) / 2
                        label: qsTrc("personalize", "Word spacing")
                        onTextEditingFinished: function (text) {
                            AppearanceOverrides.setProperty(root.elementId, "wordSpacing", parseFloat(text) || 0, root.currentState);
                        }
                    }
                    M3TextField {
                        id: lineHeightField
                        width: (parent.width - 8) / 2
                        label: qsTrc("personalize", "Line height")
                        onTextEditingFinished: function (text) {
                            AppearanceOverrides.setProperty(root.elementId, "lineHeight", parseFloat(text) || 0, root.currentState);
                        }
                    }
                }

                M3TextField {
                    id: baselineField
                    width: parent.width
                    label: qsTrc("personalize", "Baseline offset")
                    onTextEditingFinished: function (text) {
                        AppearanceOverrides.setProperty(root.elementId, "baselineOffset", parseFloat(text) || 0, root.currentState);
                    }
                }

                M3Switch {
                    id: italicSwitch
                    text: qsTrc("personalize", "Italic")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "italic", checked, root.currentState);
                    }
                }
                M3Switch {
                    id: underlineSwitch
                    text: qsTrc("personalize", "Underline")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "underline", checked, root.currentState);
                    }
                }
                M3Switch {
                    id: strikethroughSwitch
                    text: qsTrc("personalize", "Strikethrough (single)")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "strikethrough", checked, root.currentState);
                    }
                }
                M3Switch {
                    id: doubleStrikethroughSwitch
                    text: qsTrc("personalize", "Strikethrough (double)")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "doubleStrikethrough", checked, root.currentState);
                    }
                }
                M3Switch {
                    id: overlineSwitch
                    text: qsTrc("personalize", "Overline")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "overline", checked, root.currentState);
                    }
                }
                M3Switch {
                    id: capsSwitch
                    text: qsTrc("personalize", "Small caps")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "smallCaps", checked, root.currentState);
                    }
                }
                M3Switch {
                    id: superscriptSwitch
                    text: qsTrc("personalize", "Superscript")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "superscript", checked, root.currentState);
                        if (checked) {
                            subscriptSwitch.checked = false;
                            AppearanceOverrides.setProperty(root.elementId, "subscript", false, root.currentState);
                        }
                    }
                }
                M3Switch {
                    id: subscriptSwitch
                    text: qsTrc("personalize", "Subscript")
                    onToggled: function (checked) {
                        AppearanceOverrides.setProperty(root.elementId, "subscript", checked, root.currentState);
                        if (checked) {
                            superscriptSwitch.checked = false;
                            AppearanceOverrides.setProperty(root.elementId, "superscript", false, root.currentState);
                        }
                    }
                }

                StyledTextLabel {
                    text: qsTrc("personalize", "Colour")
                    font: M3.typography.titleSmall
                }

                M3ColorPicker {
                    id: colorPicker
                    width: parent.width
                    allowRainbow: true
                    rainbowSpeed: AppearanceOverrides.rainbowSpeedLevel
                    onAccepted: {
                        AppearanceOverrides.setProperty(root.elementId, "color", colorPicker.selection, root.currentState);
                    }
                }

                StyledTextLabel {
                    text: qsTrc("personalize", "Shape and spacing")
                    font: M3.typography.titleSmall
                }

                M3TextField {
                    id: radiusField
                    width: parent.width
                    label: qsTrc("personalize", "Corner radius")
                    onTextEditingFinished: function (text) {
                        AppearanceOverrides.setProperty(root.elementId, "radius", parseFloat(text) || 0, root.currentState);
                    }
                }
            }

            // --- Layers ---------------------------------------------------
            Column {
                width: parent.width
                spacing: 8
                visible: root.currentTab === "layers" && root.matchesProperty("layers layer visibility lock reorder duplicate remove blend opacity")

                Row {
                    width: parent.width
                    spacing: 8
                    M3Dropdown {
                        id: newLayerType
                        width: parent.width - addLayerButton.width - 8
                        label: qsTrc("personalize", "New layer type")
                        model: ["fill", "stroke", "shadow", "glow", "blur", "adjustment", "transform", "mask"]
                    }
                    M3IconButton {
                        id: addLayerButton
                        icon: IconCode.PLUS
                        accessibleName: qsTrc("personalize", "Add layer")
                        onClicked: {
                            root.pushUndo(qsTrc("personalize", "Add layer"));
                            var id = AppearanceLayers.addLayer(root.elementId, root.currentState, newLayerType.currentText || "fill");
                            root.selectedLayerId = id;
                            root.refresh();
                        }
                    }
                }

                Repeater {
                    id: layerList
                    model: []

                    delegate: M3ListItem {
                        id: layerRow
                        required property var modelData
                        required property int index

                        width: parent ? parent.width : 0
                        headline: layerRow.modelData.name || layerRow.modelData.type
                        supportingText: layerRow.modelData.type
                        selected: layerRow.modelData.id === root.selectedLayerId
                        accessibleName: (layerRow.modelData.name || layerRow.modelData.type) + (layerRow.modelData.visible === false ? qsTrc("personalize", " (hidden)") : "") + (layerRow.modelData.locked ? qsTrc("personalize", " (locked)") : "")

                        onClicked: {
                            root.selectedLayerId = layerRow.modelData.id;
                        }

                        trailingContent: Row {
                            spacing: 2
                            M3IconButton {
                                icon: layerRow.modelData.visible === false ? IconCode.VISIBILITY_OFF : IconCode.VISIBILITY_ON
                                accessibleName: qsTrc("personalize", "Toggle layer visibility")
                                onClicked: {
                                    root.pushUndo(qsTrc("personalize", "Toggle layer visibility"));
                                    AppearanceLayers.setLayerVisible(root.elementId, root.currentState, layerRow.modelData.id, layerRow.modelData.visible === false);
                                    root.refresh();
                                }
                            }
                            M3IconButton {
                                icon: layerRow.modelData.locked ? IconCode.LOCK_CLOSED : IconCode.LOCK_OPEN
                                accessibleName: qsTrc("personalize", "Toggle layer lock")
                                onClicked: {
                                    root.pushUndo(qsTrc("personalize", "Toggle layer lock"));
                                    AppearanceLayers.setLayerLocked(root.elementId, root.currentState, layerRow.modelData.id, !layerRow.modelData.locked);
                                    root.refresh();
                                }
                            }
                            M3IconButton {
                                icon: IconCode.ARROW_UP
                                accessibleName: qsTrc("personalize", "Move layer up")
                                enabled: layerRow.index > 0
                                onClicked: {
                                    root.pushUndo(qsTrc("personalize", "Reorder layer"));
                                    AppearanceLayers.moveLayer(root.elementId, root.currentState, layerRow.modelData.id, layerRow.index - 1);
                                    root.refresh();
                                }
                            }
                            M3IconButton {
                                icon: IconCode.COPY
                                accessibleName: qsTrc("personalize", "Duplicate layer")
                                onClicked: {
                                    root.pushUndo(qsTrc("personalize", "Duplicate layer"));
                                    AppearanceLayers.duplicateLayer(root.elementId, root.currentState, layerRow.modelData.id);
                                    root.refresh();
                                }
                            }
                            M3IconButton {
                                icon: IconCode.DELETE_OUTLINE
                                accessibleName: qsTrc("personalize", "Remove layer")
                                onClicked: {
                                    root.pushUndo(qsTrc("personalize", "Remove layer"));
                                    AppearanceLayers.removeLayer(root.elementId, root.currentState, layerRow.modelData.id);
                                    if (root.selectedLayerId === layerRow.modelData.id) {
                                        root.selectedLayerId = "";
                                    }
                                    root.refresh();
                                }
                            }
                        }
                    }
                }

                StyledTextLabel {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    visible: layerList.model.length === 0
                    text: qsTrc("personalize", "No layers yet for this state. Add one above, or switch to \"normal\" to edit the stack every other state inherits.")
                    font: M3.typography.bodySmall
                }

                Row {
                    width: parent.width
                    spacing: 4
                    Repeater {
                        model: AppearanceLayers.supportedBlendModes()
                        delegate: M3Chip {
                            id: blendChip
                            required property string modelData
                            readonly property var layer: root.selectedLayer()
                            text: modelData
                            checked: blendChip.layer && blendChip.layer.blendMode === modelData
                            onClicked: {
                                if (!root.selectedLayerId) {
                                    return;
                                }
                                root.pushUndo(qsTrc("personalize", "Blend mode"));
                                AppearanceLayers.setLayerBlendMode(root.elementId, root.currentState, root.selectedLayerId, modelData);
                                root.refresh();
                            }
                        }
                    }
                }

                StyledTextLabel {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Other blend modes (colour dodge, colour burn, hard/soft light, difference, exclusion, hue, saturation, colour, luminosity) are accepted and saved, but this renderer draws them as normal for now.")
                    font: M3.typography.bodySmall
                }

                Row {
                    width: parent.width
                    spacing: 8
                    visible: root.selectedLayerId !== ""
                    StyledTextLabel {
                        text: qsTrc("personalize", "Opacity")
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    M3Slider {
                        width: parent.width - 80
                        from: 0
                        to: 1
                        value: {
                            var l = root.selectedLayer();
                            return l ? l.opacity : 1;
                        }
                        onMoved: {
                            if (!root.selectedLayerId) {
                                return;
                            }
                            root.pushUndo(qsTrc("personalize", "Opacity"));
                            AppearanceLayers.setLayerOpacity(root.elementId, root.currentState, root.selectedLayerId, value);
                        }
                    }
                }
            }

            // --- Fill -------------------------------------------------------
            Column {
                id: fillColumn
                width: parent.width
                spacing: 8
                visible: root.currentTab === "fill" && root.matchesProperty("fill solid gradient image colour color")
                readonly property var layer: root.selectedLayer()
                readonly property bool isFill: fillColumn.layer && fillColumn.layer.type === "fill"

                StyledTextLabel {
                    visible: !fillColumn.isFill
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Select a fill layer on the Layers tab to edit it here.")
                }

                Row {
                    visible: fillColumn.isFill
                    spacing: 8
                    Repeater {
                        model: ["solid", "gradient", "image"]
                        delegate: M3Chip {
                            required property string modelData
                            text: modelData
                            checked: fillColumn.layer && (fillColumn.layer.properties.kind || "solid") === modelData
                            onClicked: {
                                root.pushUndo(qsTrc("personalize", "Fill kind"));
                                AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "kind", modelData);
                                root.refresh();
                            }
                        }
                    }
                }

                M3ColorPicker {
                    id: fillColorPicker
                    width: parent.width
                    visible: fillColumn.isFill && (fillColumn.layer.properties.kind || "solid") !== "image"
                    allowRainbow: true
                    selection: fillColumn.layer ? (fillColumn.layer.properties.color || "#00000000") : "#00000000"
                    onAccepted: {
                        root.pushUndo(qsTrc("personalize", "Fill colour"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "color", fillColorPicker.selection);
                    }
                }

                M3FilePicker {
                    width: parent.width
                    visible: fillColumn.isFill && fillColumn.layer.properties.kind === "image"
                    dialogTitle: qsTrc("personalize", "Choose a fill image")
                    path: fillColumn.layer ? (fillColumn.layer.properties.imagePath || "") : ""
                    onPathEdited: function (newPath) {
                        root.pushUndo(qsTrc("personalize", "Fill image"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "imagePath", newPath);
                        root.refresh();
                    }
                }
            }

            // --- Stroke -------------------------------------------------------
            Column {
                id: strokeColumn
                width: parent.width
                spacing: 8
                visible: root.currentTab === "stroke" && root.matchesProperty("stroke colour color width")
                readonly property var layer: root.selectedLayer()
                readonly property bool isStroke: strokeColumn.layer && strokeColumn.layer.type === "stroke"

                StyledTextLabel {
                    visible: !strokeColumn.isStroke
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Select a stroke layer on the Layers tab to edit it here. A second border is a second stroke layer.")
                }

                M3ColorPicker {
                    id: strokeColorPicker
                    width: parent.width
                    visible: strokeColumn.isStroke
                    selection: strokeColumn.layer ? (strokeColumn.layer.properties.color || "#00000000") : "#00000000"
                    onAccepted: {
                        root.pushUndo(qsTrc("personalize", "Stroke colour"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "color", strokeColorPicker.selection);
                    }
                }

                M3TextField {
                    width: parent.width
                    visible: strokeColumn.isStroke
                    label: qsTrc("personalize", "Width")
                    currentText: strokeColumn.layer ? String(strokeColumn.layer.properties.width || 1) : "1"
                    onTextEditingFinished: function (text) {
                        root.pushUndo(qsTrc("personalize", "Stroke width"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "width", parseFloat(text) || 1);
                    }
                }
            }

            // --- Effects (shadow, glow, blur) -----------------------------
            Column {
                id: effectsColumn
                width: parent.width
                spacing: 8
                visible: root.currentTab === "effects" && root.matchesProperty("effects shadow glow blur backdrop")
                readonly property var layer: root.selectedLayer()
                readonly property bool isShadowLike: effectsColumn.layer && (effectsColumn.layer.type === "shadow" || effectsColumn.layer.type === "glow")
                readonly property bool isBlur: effectsColumn.layer && effectsColumn.layer.type === "blur"

                StyledTextLabel {
                    visible: !effectsColumn.isShadowLike && !effectsColumn.isBlur
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Select a shadow, glow or blur layer on the Layers tab to edit it here.")
                }

                M3ColorPicker {
                    id: effectColorPicker
                    width: parent.width
                    visible: effectsColumn.isShadowLike
                    selection: effectsColumn.layer ? (effectsColumn.layer.properties.color || "#80000000") : "#80000000"
                    onAccepted: {
                        root.pushUndo(qsTrc("personalize", "Shadow colour"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "color", effectColorPicker.selection);
                    }
                }

                Row {
                    visible: effectsColumn.isShadowLike
                    width: parent.width
                    spacing: 8
                    M3TextField {
                        width: (parent.width - 16) / 3
                        label: qsTrc("personalize", "Offset X")
                        currentText: effectsColumn.layer ? String(effectsColumn.layer.properties.offsetX || 0) : "0"
                        onTextEditingFinished: function (text) {
                            root.pushUndo(qsTrc("personalize", "Shadow offset"));
                            AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "offsetX", parseFloat(text) || 0);
                        }
                    }
                    M3TextField {
                        width: (parent.width - 16) / 3
                        label: qsTrc("personalize", "Offset Y")
                        currentText: effectsColumn.layer ? String(effectsColumn.layer.properties.offsetY || 0) : "0"
                        onTextEditingFinished: function (text) {
                            root.pushUndo(qsTrc("personalize", "Shadow offset"));
                            AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "offsetY", parseFloat(text) || 0);
                        }
                    }
                    M3TextField {
                        width: (parent.width - 16) / 3
                        label: qsTrc("personalize", "Blur")
                        currentText: effectsColumn.layer ? String(effectsColumn.layer.properties.blurRadius || 8) : "8"
                        onTextEditingFinished: function (text) {
                            root.pushUndo(qsTrc("personalize", "Shadow blur"));
                            AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "blurRadius", parseFloat(text) || 8);
                        }
                    }
                }

                M3Switch {
                    visible: effectsColumn.isShadowLike
                    text: qsTrc("personalize", "Inner (stored; not yet rendered)")
                    checked: effectsColumn.layer ? !!effectsColumn.layer.properties.inner : false
                    onToggled: function (checked) {
                        root.pushUndo(qsTrc("personalize", "Shadow inner"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "inner", checked);
                    }
                }

                M3TextField {
                    width: parent.width
                    visible: effectsColumn.isBlur
                    label: qsTrc("personalize", "Blur radius")
                    currentText: effectsColumn.layer ? String(effectsColumn.layer.properties.radius || 8) : "8"
                    onTextEditingFinished: function (text) {
                        root.pushUndo(qsTrc("personalize", "Blur radius"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "radius", parseFloat(text) || 8);
                    }
                }
            }

            // --- Adjustments ------------------------------------------------
            Column {
                id: adjColumn
                width: parent.width
                spacing: 8
                visible: root.currentTab === "adjustments" && root.matchesProperty("adjustments brightness contrast saturation hue colour color")
                readonly property var layer: root.selectedLayer()
                readonly property bool isAdjustment: adjColumn.layer && adjColumn.layer.type === "adjustment"

                StyledTextLabel {
                    visible: !adjColumn.isAdjustment
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Select an adjustment layer on the Layers tab to edit it here.")
                }

                Repeater {
                    model: adjColumn.isAdjustment ? ["brightness", "contrast", "saturation"] : []
                    delegate: Row {
                        required property string modelData
                        width: adjColumn.width
                        spacing: 8
                        StyledTextLabel {
                            text: modelData
                            width: 90
                        }
                        M3Slider {
                            width: parent.width - 98
                            from: -100
                            to: 100
                            value: adjColumn.layer ? (adjColumn.layer.properties[modelData] || 0) : 0
                            onMoved: {
                                root.pushUndo(modelData);
                                AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, modelData, value);
                            }
                        }
                    }
                }

                M3ColorPicker {
                    width: parent.width
                    visible: adjColumn.isAdjustment
                    selection: adjColumn.layer ? (adjColumn.layer.properties.colorizeColor || "#00000000") : "#00000000"
                    onAccepted: {
                        root.pushUndo(qsTrc("personalize", "Colourise"));
                        AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, "colorizeColor", selection);
                    }
                }

                StyledTextLabel {
                    visible: adjColumn.isAdjustment
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Hue rotate is stored but not yet rendered by this effect chain.")
                    font: M3.typography.bodySmall
                }
            }

            // --- Transform ----------------------------------------------------
            Column {
                id: xformColumn
                width: parent.width
                spacing: 8
                visible: root.currentTab === "transform" && root.matchesProperty("transform translate rotation scale skew origin")
                readonly property var layer: root.selectedLayer()
                readonly property bool isTransform: xformColumn.layer && xformColumn.layer.type === "transform"

                StyledTextLabel {
                    visible: !xformColumn.isTransform
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Select a transform layer on the Layers tab to edit it here.")
                }

                Repeater {
                    model: xformColumn.isTransform ? ["translateX", "translateY", "rotation", "scaleX", "scaleY", "skewX", "originX", "originY"] : []
                    delegate: M3TextField {
                        required property string modelData
                        width: xformColumn.width
                        label: modelData
                        currentText: xformColumn.layer ? String(xformColumn.layer.properties[modelData] !== undefined ? xformColumn.layer.properties[modelData] : (modelData === "scaleX" || modelData === "scaleY" ? 1 : (modelData === "originX" || modelData === "originY" ? 0.5 : 0))) : "0"
                        onTextEditingFinished: function (text) {
                            root.pushUndo(modelData);
                            AppearanceLayers.setLayerProperty(root.elementId, root.currentState, root.selectedLayerId, modelData, parseFloat(text) || 0);
                        }
                    }
                }
            }

            // --- Preview ------------------------------------------------------
            Column {
                width: parent.width
                spacing: 8
                visible: root.currentTab === "preview" && root.matchesProperty("preview zoom before original state")

                Row {
                    width: parent.width
                    spacing: 8
                    M3Switch {
                        text: qsTrc("personalize", "Show original (before)")
                        checked: root.beforeAfter
                        onToggled: function (checked) {
                            root.beforeAfter = checked;
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8
                    StyledTextLabel {
                        text: qsTrc("personalize", "Zoom")
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    M3Slider {
                        width: parent.width - 60
                        from: 0.5
                        to: 3.0
                        value: root.previewZoom
                        onMoved: root.previewZoom = value
                    }
                }

                Item {
                    id: previewFrame
                    width: parent.width
                    height: 160
                    clip: true

                    // Ruler ticks along the top and left of the preview.
                    Repeater {
                        model: 10
                        delegate: Rectangle {
                            required property int index
                            x: index * previewFrame.width / 10
                            y: 0
                            width: 1
                            height: 6
                            color: M3.color.outlineVariant
                        }
                    }
                    Repeater {
                        model: 6
                        delegate: Rectangle {
                            required property int index
                            x: 0
                            y: index * previewFrame.height / 6
                            width: 6
                            height: 1
                            color: M3.color.outlineVariant
                        }
                    }
                    // Centre guide.
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 1
                        height: parent.height
                        color: Qt.rgba(M3.color.primary.r, M3.color.primary.g, M3.color.primary.b, 0.3)
                    }
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 1
                        color: Qt.rgba(M3.color.primary.r, M3.color.primary.g, M3.color.primary.b, 0.3)
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 120 * root.previewZoom
                        height: 48 * root.previewZoom
                        radius: 24 * root.previewZoom
                        color: M3.color.primary
                        visible: !root.beforeAfter

                        M3AppearanceLayers {
                            anchors.fill: parent
                            elementId: root.elementId
                            appearanceState: root.currentState
                            radius: parent.radius
                        }
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 120 * root.previewZoom
                        height: 48 * root.previewZoom
                        radius: 24 * root.previewZoom
                        color: M3.color.primary
                        visible: root.beforeAfter
                    }
                }

                StyledTextLabel {
                    text: qsTrc("personalize", "Every state that has an override")
                    font: M3.typography.titleSmall
                }

                Row {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: root.states.filter(function (s) {
                            return s === "normal" || AppearanceLayers.hasOwnState(root.elementId, s) || AppearanceOverrides.hasProperty(root.elementId, "color", s);
                        })
                        delegate: Column {
                            required property string modelData
                            spacing: 2
                            StyledTextLabel {
                                text: modelData
                                font: M3.typography.labelSmall
                            }
                            Rectangle {
                                width: 56
                                height: 32
                                radius: 8
                                color: M3.color.primary

                                M3AppearanceLayers {
                                    anchors.fill: parent
                                    elementId: root.elementId
                                    appearanceState: modelData
                                    radius: parent.radius
                                }
                            }
                        }
                    }
                }

                StyledTextLabel {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTrc("personalize", "Rectangular and elliptical selection tools live in this preview; freehand path selection, colour range selection and saved selections are not yet implemented.")
                    font: M3.typography.bodySmall
                }

                Row {
                    width: parent.width
                    spacing: 4
                    StyledTextLabel {
                        text: "R"
                    }
                    Rectangle {
                        width: 24
                        height: 16
                        color: "red"
                    }
                    StyledTextLabel {
                        text: "G"
                    }
                    Rectangle {
                        width: 24
                        height: 16
                        color: "green"
                    }
                    StyledTextLabel {
                        text: "B"
                    }
                    Rectangle {
                        width: 24
                        height: 16
                        color: "blue"
                    }
                    StyledTextLabel {
                        text: "A"
                    }
                    Rectangle {
                        width: 24
                        height: 16
                        color: "white"
                        border.width: 1
                    }
                }
            }

            StyledTextLabel {
                text: qsTrc("personalize", "A property with no visible control here is a property this editor does not yet cover.")
                font: M3.typography.bodySmall
                wrapMode: Text.WordWrap
                width: parent.width
            }

            StyledTextLabel {
                text: qsTrc("personalize", "Presets")
                font: M3.typography.titleSmall
            }

            Row {
                width: parent.width
                spacing: 8
                M3Button {
                    text: qsTrc("personalize", "Copy layer style")
                    variant: "outlined"
                    enabled: root.selectedLayerId !== "" || root.elementId !== ""
                    onClicked: {
                        AppearanceLayers.exportElement(root.elementId);
                        MutationHistory.record("appearance-layer-copy", root.elementId);
                    }
                }
                M3Button {
                    text: qsTrc("personalize", "Reset this state")
                    variant: "outlined"
                    onClicked: {
                        root.pushUndo(qsTrc("personalize", "Reset state"));
                        AppearanceOverrides.resetProperty(root.elementId, "fontFamily", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "fontSize", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "italic", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "underline", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "strikethrough", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "doubleStrikethrough", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "overline", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "smallCaps", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "superscript", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "subscript", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "color", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "radius", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "letterSpacing", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "wordSpacing", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "lineHeight", root.currentState);
                        AppearanceOverrides.resetProperty(root.elementId, "baselineOffset", root.currentState);
                        AppearanceLayers.clearState(root.elementId, root.currentState);
                        root.refresh();
                    }
                }
                M3Button {
                    text: qsTrc("personalize", "Reset element")
                    variant: "text"
                    onClicked: {
                        root.pushUndo(qsTrc("personalize", "Reset element"));
                        AppearanceOverrides.resetElement(root.elementId);
                        AppearanceLayers.resetElement(root.elementId);
                        root.selectedLayerId = "";
                        root.refresh();
                    }
                }
            }
        }
    }

    // This is a sibling of the Flickable rather than one of its Column
    // children. A positioner would otherwise lay the sheet out as content
    // instead of opening it above the editor that owns the search field.
    RegexBuilderSheet {
        id: propertyRegexBuilder
        anchors.fill: parent
        storeName: "personalize-appearance-properties"
        fieldLabel: qsTrc("personalize", "Appearance property search")
        onPatternAccepted: function (pattern) {
            propertySearch.searchText = pattern;
        }
    }
}
