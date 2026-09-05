/*
* Audacity: A Digital Audio Editor
*
* M3ComponentsGallery
*
* Shows every Material Design 3 component from Audacity.M3 in each of its
* variants and states.
*
* Deterministic capture:
* set AU_M3_GALLERY_ROUTE to "<component>:<state>:<theme>:<scale>" and the
* gallery selects that entry on load, for example
*
*     AU_M3_GALLERY_ROUTE="M3Button:hover:dark:1.5"
*
* The component and state parts choose the entry, the theme part is reported
* back so a capture harness can assert the theme it asked for was applied, and
* the scale part sets the preview scale. Any part may be left empty.
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    color: M3.color.surface
    clip: true

    // The parsed AU_M3_GALLERY_ROUTE, empty when the variable is not set.
    property string routeComponent: ""
    property string routeState: ""
    property string routeTheme: ""
    property real routeScale: 1.0

    readonly property var entries: [
        { "name": "M3Button", "component": buttonsPage },
        { "name": "M3IconButton", "component": iconButtonsPage },
        { "name": "M3FAB", "component": fabPage },
        { "name": "M3SegmentedButton", "component": segmentedPage },
        { "name": "M3Switch", "component": switchPage },
        { "name": "M3Checkbox", "component": checkboxPage },
        { "name": "M3RadioButton", "component": radioPage },
        { "name": "M3Slider", "component": sliderPage },
        { "name": "M3RangeSlider", "component": rangeSliderPage },
        { "name": "M3TextField", "component": textFieldPage },
        { "name": "M3SearchBar", "component": searchBarPage },
        { "name": "M3Menu", "component": menuPage },
        { "name": "M3Dropdown", "component": dropdownPage },
        { "name": "M3Chip", "component": chipPage },
        { "name": "M3Card", "component": cardPage },
        { "name": "M3Dialog", "component": dialogPage },
        { "name": "M3BottomSheet", "component": bottomSheetPage },
        { "name": "M3SideSheet", "component": sideSheetPage },
        { "name": "M3Snackbar", "component": snackbarPage },
        { "name": "M3Tooltip", "component": tooltipPage },
        { "name": "M3TopAppBar", "component": topAppBarPage },
        { "name": "M3NavigationRail", "component": navigationRailPage },
        { "name": "M3NavigationDrawer", "component": navigationDrawerPage },
        { "name": "M3Tabs", "component": tabsPage },
        { "name": "M3LinearProgress", "component": linearProgressPage },
        { "name": "M3CircularProgress", "component": circularProgressPage },
        { "name": "M3Badge", "component": badgePage },
        { "name": "M3Divider", "component": dividerPage },
        { "name": "M3ListItem", "component": listItemPage },
        { "name": "M3DatePicker", "component": datePickerPage },
        { "name": "M3TimePicker", "component": timePickerPage },
        { "name": "M3ColorPicker", "component": colorPickerPage },
        { "name": "M3Surface", "component": surfacePage },
        { "name": "M3StateLayer", "component": stateLayerPage },
        { "name": "Tokens", "component": tokensPage }
    ]

    property int currentIndex: 0

    NavigationSection {
        id: navSec

        name: "M3ComponentsGallery"
        enabled: root.visible
        order: 5
    }

    NavigationPanel {
        id: listPanel

        name: "M3GalleryList"
        section: navSec
        order: 1
        direction: NavigationPanel.Vertical
    }

    NavigationPanel {
        id: contentPanel

        name: "M3GalleryContent"
        section: navSec
        order: 2
        direction: NavigationPanel.Both
    }

    function selectByName(name) {
        for (var i = 0; i < root.entries.length; ++i) {
            if (root.entries[i].name === name) {
                root.currentIndex = i
                return true
            }
        }
        return false
    }

    Component.onCompleted: {
        var route = M3.captureRoute()
        if (route === "") {
            return
        }

        var parts = route.split(":")
        root.routeComponent = parts.length > 0 ? parts[0] : ""
        root.routeState = parts.length > 1 ? parts[1] : ""
        root.routeTheme = parts.length > 2 ? parts[2] : ""
        root.routeScale = parts.length > 3 && parts[3] !== "" ? parseFloat(parts[3]) : 1.0

        if (root.routeComponent !== "") {
            root.selectByName(root.routeComponent)
        }

        if (root.routeTheme !== "") {
            M3.applyScheme(root.routeTheme)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Component list.
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: M3.color.surfaceContainerLow

            ListView {
                id: list

                anchors.fill: parent
                clip: true
                model: root.entries
                currentIndex: root.currentIndex

                delegate: M3ListItem {
                    required property int index
                    required property var modelData

                    width: list.width
                    headline: modelData.name
                    selected: root.currentIndex === index
                    navigation.panel: listPanel
                    navigation.row: index

                    onClicked: root.currentIndex = index
                }
            }
        }

        M3Divider {
            Layout.fillHeight: true
            orientation: Qt.Vertical
        }

        // Component page.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Flickable {
                id: pageFlickable

                anchors.fill: parent
                anchors.margins: 24
                contentHeight: pageLoader.height * root.routeScale
                contentWidth: width

                Item {
                    width: pageFlickable.width / root.routeScale
                    height: pageLoader.height
                    scale: root.routeScale
                    transformOrigin: Item.TopLeft

                    Loader {
                        id: pageLoader

                        width: parent.width
                        height: pageLoader.item ? (pageLoader.item as Item).height : 0
                        sourceComponent: root.entries[root.currentIndex].component
                    }
                }
            }

            // Reported back to the capture harness.
            StyledTextLabel {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                visible: root.routeComponent !== ""
                text: "route " + root.routeComponent + " state " + root.routeState
                      + " theme " + root.routeTheme + " requested, " + M3.schemeName + " applied"
                font: M3.typography.labelSmall
                color: M3.color.onSurfaceVariant
            }
        }
    }

    // ------------------------------------------------------------------
    // Component pages
    // ------------------------------------------------------------------

    component Section: Column {
        property string title: ""

        width: parent ? parent.width : 0
        spacing: 12

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            text: parent.title
            font: M3.typography.titleMedium
            color: M3.color.onSurface
        }
    }

    Component {
        id: buttonsPage

        Column {
            spacing: 24

            Repeater {
                model: ["filled", "tonal", "outlined", "text", "elevated"]

                delegate: Column {
                    id: buttonRow

                    required property var modelData

                    spacing: 8

                    StyledTextLabel {
                        horizontalAlignment: Text.AlignLeft
                        text: buttonRow.modelData
                        font: M3.typography.titleSmall
                        color: M3.color.onSurfaceVariant
                    }

                    Row {
                        spacing: 12

                        M3Button {
                            variant: buttonRow.modelData
                            text: "Enabled"
                            navigation.panel: contentPanel
                        }

                        M3Button {
                            variant: buttonRow.modelData
                            text: "With icon"
                            icon: IconCode.PLAY
                            navigation.panel: contentPanel
                        }

                        M3Button {
                            variant: buttonRow.modelData
                            text: "Loading"
                            loading: true
                        }

                        M3Button {
                            variant: buttonRow.modelData
                            text: "Disabled"
                            enabled: false
                        }
                    }
                }
            }
        }
    }

    Component {
        id: iconButtonsPage

        Column {
            spacing: 16

            Repeater {
                model: ["standard", "filled", "tonal", "outlined"]

                delegate: Row {
                    id: iconButtonRow

                    required property var modelData

                    spacing: 12

                    StyledTextLabel {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 100
                        horizontalAlignment: Text.AlignLeft
                        text: iconButtonRow.modelData
                        font: M3.typography.titleSmall
                        color: M3.color.onSurfaceVariant
                    }

                    M3IconButton {
                        variant: iconButtonRow.modelData
                        icon: IconCode.PLAY
                        accessibleName: "Play"
                        navigation.panel: contentPanel
                    }

                    M3IconButton {
                        variant: iconButtonRow.modelData
                        icon: IconCode.STAR
                        checkable: true
                        checked: true
                        accessibleName: "Favourite"
                    }

                    M3IconButton {
                        variant: iconButtonRow.modelData
                        icon: IconCode.STAR
                        checkable: true
                        accessibleName: "Favourite"
                    }

                    M3IconButton {
                        variant: iconButtonRow.modelData
                        icon: IconCode.PLAY
                        enabled: false
                        accessibleName: "Play"
                    }
                }
            }
        }
    }

    Component {
        id: fabPage

        Row {
            spacing: 24

            M3FAB { size: "small"; icon: IconCode.PLUS; accessibleName: "Add" }
            M3FAB { size: "regular"; icon: IconCode.PLUS; accessibleName: "Add" }
            M3FAB { size: "large"; icon: IconCode.PLUS; accessibleName: "Add" }
            M3FAB { size: "extended"; icon: IconCode.PLUS; text: "New track" }
            M3FAB { size: "regular"; icon: IconCode.PLUS; variant: "secondary"; accessibleName: "Add" }
            M3FAB { size: "regular"; icon: IconCode.PLUS; variant: "tertiary"; accessibleName: "Add" }
            M3FAB { size: "regular"; icon: IconCode.PLUS; variant: "surface"; accessibleName: "Add" }
        }
    }

    Component {
        id: segmentedPage

        Column {
            spacing: 16

            M3SegmentedButton {
                model: ["Day", "Week", "Month"]
                navigationPanel: contentPanel
            }

            M3SegmentedButton {
                model: [{ "text": "Loop", "icon": IconCode.LOOP },
                        { "text": "Solo", "icon": IconCode.PLAY },
                        { "text": "Mute", "icon": IconCode.MUTE }]
                multiSelect: true
                checkedIndexes: [0]
                navigationPanel: contentPanel
            }
        }
    }

    Component {
        id: switchPage

        Column {
            spacing: 16

            M3Switch { text: "On with icon"; checked: true; navigation.panel: contentPanel }
            M3Switch { text: "Off with icon" }
            M3Switch { text: "On without icon"; checked: true; showIcon: false }
            M3Switch { text: "Disabled"; enabled: false }
            M3Switch { text: "Disabled and on"; checked: true; enabled: false }
        }
    }

    Component {
        id: checkboxPage

        Column {
            spacing: 8

            M3Checkbox { text: "Unchecked"; navigation.panel: contentPanel }
            M3Checkbox { text: "Checked"; checked: true }
            M3Checkbox { text: "Indeterminate"; indeterminate: true }
            M3Checkbox { text: "Disabled"; enabled: false }
            M3Checkbox { text: "Disabled and checked"; checked: true; enabled: false }
        }
    }

    Component {
        id: radioPage

        Column {
            spacing: 8

            M3RadioButton { text: "Selected"; checked: true; navigation.panel: contentPanel }
            M3RadioButton { text: "Not selected" }
            M3RadioButton { text: "Disabled"; enabled: false }
        }
    }

    Component {
        id: sliderPage

        Row {
            spacing: 32

            Column {
                spacing: 24

                M3Slider {
                    width: 240
                    value: 0.4
                    accessibleName: "Continuous"
                    navigation.panel: contentPanel
                }

                M3Slider {
                    width: 240
                    from: 0
                    to: 10
                    stepSize: 1
                    value: 6
                    accessibleName: "Discrete"
                }

                M3Slider {
                    width: 240
                    value: 0.4
                    enabled: false
                    accessibleName: "Disabled"
                }
            }

            M3Slider {
                height: 200
                orientation: Qt.Vertical
                value: 0.7
                accessibleName: "Vertical"
            }
        }
    }

    Component {
        id: rangeSliderPage

        Column {
            spacing: 24

            M3RangeSlider {
                width: 280
                first: 0.2
                second: 0.8
                navigationPanel: contentPanel
            }

            M3RangeSlider {
                height: 180
                orientation: Qt.Vertical
                first: 0.3
                second: 0.7
            }
        }
    }

    Component {
        id: textFieldPage

        Column {
            spacing: 20

            M3TextField {
                width: 280
                variant: "outlined"
                label: "Outlined"
                supportingText: "Supporting text"
                navigation.panel: contentPanel
            }

            M3TextField {
                width: 280
                variant: "filled"
                label: "Filled"
                currentText: "With content"
            }

            M3TextField {
                width: 280
                label: "With icons"
                leadingIcon: IconCode.SEARCH
                trailingIcon: IconCode.CLOSE_X_ROUNDED
            }

            M3TextField {
                width: 280
                label: "Error"
                currentText: "Not valid"
                errorText: "This value is not valid"
            }

            M3TextField {
                width: 280
                label: "Password"
                isPassword: true
                currentText: "secret"
            }

            M3TextField {
                width: 280
                label: "Counted"
                maximumLength: 40
                currentText: "Counted text"
            }

            M3TextField {
                width: 280
                label: "Disabled"
                enabled: false
            }
        }
    }

    Component {
        id: searchBarPage

        Column {
            spacing: 16

            M3SearchBar {
                width: 360
                navigation.panel: contentPanel
            }

            M3SearchBar {
                width: 360
                searchText: "reverb"
                showRegexBuilder: true
            }
        }
    }

    Component {
        id: menuPage

        Item {
            implicitHeight: 260

            M3Button {
                id: menuButton

                text: "Open menu"
                navigation.panel: contentPanel
                onClicked: sampleMenu.toggleOpened()
            }

            M3Menu {
                id: sampleMenu

                parent: menuButton
                y: menuButton.height
                searchable: true
                navigationPanel: contentPanel

                model: [
                    { "id": "undo", "title": "Undo", "shortcut": "Ctrl+Z", "icon": IconCode.UNDO },
                    { "id": "redo", "title": "Redo", "shortcut": "Ctrl+Shift+Z", "icon": IconCode.REDO },
                    { "separator": true },
                    { "id": "loop", "title": "Loop playback", "checkable": true, "checked": true },
                    { "id": "more", "title": "More", "subitems": [
                        { "id": "a", "title": "First" },
                        { "id": "b", "title": "Second" }
                    ] },
                    { "id": "off", "title": "Not available", "enabled": false }
                ]
            }
        }
    }

    Component {
        id: dropdownPage

        Column {
            spacing: 16

            M3Dropdown {
                width: 240
                label: "Sample rate"
                model: [{ "text": "44100 Hz", "value": 44100 },
                        { "text": "48000 Hz", "value": 48000 },
                        { "text": "96000 Hz", "value": 96000 }]
                currentIndex: 1
                navigation.panel: contentPanel
            }

            M3Dropdown {
                width: 240
                label: "Disabled"
                model: ["One", "Two"]
                enabled: false
            }
        }
    }

    Component {
        id: chipPage

        Column {
            spacing: 16

            Row {
                spacing: 8

                M3Chip { text: "Assist"; variant: "assist"; icon: IconCode.STAR; navigation.panel: contentPanel }
                M3Chip { text: "Filter"; variant: "filter" }
                M3Chip { text: "Filter on"; variant: "filter"; checked: true }
                M3Chip { text: "Input"; variant: "input" }
                M3Chip { text: "Suggestion"; variant: "suggestion" }
                M3Chip { text: "Elevated"; elevated: true }
                M3Chip { text: "Disabled"; enabled: false }
            }
        }
    }

    Component {
        id: cardPage

        Row {
            spacing: 16

            Repeater {
                model: ["elevated", "filled", "outlined"]

                delegate: M3Card {
                    id: sampleCard

                    required property var modelData

                    width: 200
                    height: 140
                    variant: sampleCard.modelData
                    clickable: true
                    accessibleName: sampleCard.modelData + " card"
                    navigation.panel: contentPanel

                    StyledTextLabel {
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignTop
                        wrapMode: Text.WordWrap
                        text: sampleCard.modelData + " card. Click it to see the state layer and the ripple."
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurface
                    }
                }
            }
        }
    }

    Component {
        id: dialogPage

        Column {
            spacing: 16

            StyledTextLabel {
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                width: 480
                text: "M3Dialog is built on the muse StyledDialogView, so it is opened through the "
                      + "interactive provider by URI rather than shown inline. The gallery shows its "
                      + "anatomy instead."
                font: M3.typography.bodyMedium
                color: M3.color.onSurfaceVariant
            }

            M3Card {
                width: 420
                height: 220
                variant: "elevated"

                Column {
                    anchors.fill: parent
                    spacing: 16

                    StyledIconLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        iconCode: IconCode.INFO
                        color: M3.color.secondary
                    }

                    StyledTextLabel {
                        width: parent.width
                        text: "Discard the recording?"
                        font: M3.typography.headlineSmall
                        color: M3.color.onSurface
                    }

                    StyledTextLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "This recording has not been saved. Discarding it cannot be undone."
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurfaceVariant
                    }

                    Row {
                        anchors.right: parent.right
                        spacing: 8

                        M3Button { variant: "text"; text: "Cancel" }
                        M3Button { variant: "filled"; text: "Discard" }
                    }
                }
            }
        }
    }

    Component {
        id: bottomSheetPage

        Item {
            implicitHeight: 380

            M3Button {
                text: "Open bottom sheet"
                navigation.panel: contentPanel
                onClicked: bottomSheet.open()
            }

            Item {
                anchors.fill: parent

                M3BottomSheet {
                    id: bottomSheet

                    headline: "Export options"

                    M3ListItem { width: parent.width; headline: "WAV"; supportingText: "Uncompressed" }
                    M3ListItem { width: parent.width; headline: "FLAC"; supportingText: "Lossless"; y: 72 }
                }
            }
        }
    }

    Component {
        id: sideSheetPage

        Item {
            implicitHeight: 380

            M3Button {
                text: "Open side sheet"
                navigation.panel: contentPanel
                onClicked: sideSheet.open()
            }

            Item {
                anchors.fill: parent

                M3SideSheet {
                    id: sideSheet

                    headline: "Track settings"

                    M3Switch { text: "Solo" }
                }
            }
        }
    }

    Component {
        id: snackbarPage

        Item {
            implicitHeight: 200

            M3Button {
                text: "Show snackbar"
                navigation.panel: contentPanel
                onClicked: snackbarHost.show("Track deleted", "Undo", 4000)
            }

            M3SnackbarHost {
                id: snackbarHost

                y: 100
            }
        }
    }

    Component {
        id: tooltipPage

        Row {
            spacing: 24

            M3Tooltip {
                text: "A plain tooltip"
                visible: true
            }

            M3Tooltip {
                subhead: "Rich tooltip"
                supportingText: "Rich tooltips explain a feature and can carry up to two actions."
                actionText: "Learn more"
                visible: true
            }
        }
    }

    Component {
        id: topAppBarPage

        Column {
            spacing: 24

            Repeater {
                model: ["small", "centerAligned", "medium", "large"]

                delegate: M3TopAppBar {
                    required property var modelData

                    width: 520
                    size: modelData
                    title: modelData
                    navigationIcon: IconCode.MENU_THREE_DOTS
                    navigationPanel: contentPanel

                    M3IconButton { icon: IconCode.SETTINGS_COG; accessibleName: "Settings" }
                }
            }
        }
    }

    Component {
        id: navigationRailPage

        M3NavigationRail {
            implicitHeight: 400
            fabIcon: IconCode.PLUS
            navigationPanel: contentPanel
            model: [{ "text": "Tracks", "icon": IconCode.AUDIO },
                    { "text": "Effects", "icon": IconCode.CONFIGURE },
                    { "text": "Mixer", "icon": IconCode.MIXER }]
        }
    }

    Component {
        id: navigationDrawerPage

        M3NavigationDrawer {
            implicitHeight: 400
            headline: "Audacity"
            navigationPanel: contentPanel
            model: [{ "text": "Tracks", "icon": IconCode.AUDIO, "badgeCount": 3 },
                    { "text": "Effects", "icon": IconCode.CONFIGURE },
                    { "separator": true },
                    { "headline": "Recent" },
                    { "text": "Podcast", "icon": IconCode.AUDIO }]
        }
    }

    Component {
        id: tabsPage

        Column {
            spacing: 24

            M3Tabs {
                width: 480
                primary: true
                navigationPanel: contentPanel
                model: [{ "text": "Waveform" }, { "text": "Spectrogram" }, { "text": "Notes", "badgeCount": 2 }]
            }

            M3Tabs {
                width: 480
                primary: false
                model: ["First", "Second", "Third"]
            }

            M3Tabs {
                width: 200
                height: 200
                orientation: Qt.Vertical
                model: ["Docked one", "Docked two", "Docked three"]
            }
        }
    }

    Component {
        id: linearProgressPage

        Column {
            spacing: 24

            M3LinearProgress { width: 320; value: 0.6; accessibleName: "Determinate" }
            M3LinearProgress { width: 320; indeterminate: true; accessibleName: "Indeterminate" }
            M3LinearProgress { width: 320; value: 0.6; wavy: true; accessibleName: "Wavy" }
        }
    }

    Component {
        id: circularProgressPage

        Row {
            spacing: 24

            M3CircularProgress { indeterminate: false; value: 0.7; accessibleName: "Determinate" }
            M3CircularProgress { indeterminate: true; accessibleName: "Indeterminate" }
            M3CircularProgress { indeterminate: true; wavy: true; implicitSize: 64 }
        }
    }

    Component {
        id: badgePage

        Row {
            spacing: 24

            M3Badge { showCount: false }
            M3Badge { count: 3 }
            M3Badge { count: 42 }
            M3Badge { count: 1200; maxCount: 999 }
        }
    }

    Component {
        id: dividerPage

        Column {
            spacing: 24

            M3Divider { width: 320 }
            M3Divider { width: 320; inset: 16 }

            Row {
                height: 60
                spacing: 24

                M3Divider { height: 60; orientation: Qt.Vertical }
            }
        }
    }

    Component {
        id: listItemPage

        Column {
            spacing: 0

            M3ListItem {
                width: 400
                headline: "One line"
                leadingIcon: IconCode.AUDIO
                navigation.panel: contentPanel
            }

            M3ListItem {
                width: 400
                headline: "Two lines"
                supportingText: "With supporting text"
                leadingIcon: IconCode.AUDIO
                trailingText: "3:20"
            }

            M3ListItem {
                width: 400
                overline: "Overline"
                headline: "Three lines"
                supportingText: "With an overline and supporting text"
                leadingIcon: IconCode.AUDIO
            }

            M3ListItem {
                width: 400
                headline: "Selected"
                selected: true
                leadingIcon: IconCode.AUDIO
            }
        }
    }

    Component {
        id: datePickerPage

        M3DatePicker {
            navigationPanel: contentPanel
        }
    }

    Component {
        id: timePickerPage

        Column {
            spacing: 24

            M3TimePicker {
                hours: 14
                minutes: 30
                navigationPanel: contentPanel
            }

            M3TimePicker {
                hours: 14
                minutes: 30
                use24Hour: false
            }
        }
    }

    Component {
        id: colorPickerPage

        M3ColorPicker {
            navigationPanel: contentPanel
        }
    }

    Component {
        id: surfacePage

        Row {
            spacing: 16

            Repeater {
                model: 6

                delegate: M3Surface {
                    id: sampleSurface

                    required property int index

                    width: 100
                    height: 100
                    level: sampleSurface.index
                    radius: M3.shape.medium

                    StyledTextLabel {
                        anchors.centerIn: parent
                        text: "Level " + sampleSurface.index
                        font: M3.typography.labelMedium
                        color: M3.color.onSurface
                    }
                }
            }
        }
    }

    Component {
        id: stateLayerPage

        Row {
            spacing: 16

            Repeater {
                model: [{ "name": "hover", "hovered": true },
                        { "name": "focus", "focused": true },
                        { "name": "pressed", "pressed": true },
                        { "name": "dragged", "dragged": true }]

                delegate: Rectangle {
                    id: stateSample

                    required property var modelData

                    width: 100
                    height: 100
                    radius: M3.shape.medium
                    color: M3.color.primaryContainer

                    M3StateLayer {
                        anchors.fill: parent
                        radius: stateSample.radius
                        color: M3.color.onPrimaryContainer
                        hovered: stateSample.modelData.hovered === true
                        focused: stateSample.modelData.focused === true
                        pressed: stateSample.modelData.pressed === true
                        dragged: stateSample.modelData.dragged === true
                    }

                    StyledTextLabel {
                        anchors.centerIn: parent
                        text: stateSample.modelData.name
                        font: M3.typography.labelMedium
                        color: M3.color.onPrimaryContainer
                    }
                }
            }
        }
    }

    Component {
        id: tokensPage

        Column {
            spacing: 16

            StyledTextLabel {
                horizontalAlignment: Text.AlignLeft
                text: "Scheme " + M3.schemeName + ", seed " + M3.seedColor
                      + ", variant " + M3.variant
                      + ", reduced motion " + M3.motion.reducedMotion
                      + ", density " + M3.density.level
                font: M3.typography.titleMedium
                color: M3.color.onSurface
            }

            Flow {
                width: 720
                spacing: 8

                Repeater {
                    model: M3.color.roleNames()

                    delegate: Rectangle {
                        id: swatch

                        required property var modelData

                        width: 160
                        height: 56
                        radius: M3.shape.extraSmall
                        color: M3.color.roles[swatch.modelData]
                        border.width: 1
                        border.color: M3.color.outlineVariant

                        StyledTextLabel {
                            anchors.fill: parent
                            anchors.margins: 4
                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.WordWrap
                            text: swatch.modelData
                            font: M3.typography.labelSmall
                            color: M3.contrastRatio(M3.color.onSurface,
                                                    M3.color.roles[swatch.modelData]) >= 4.5
                                   ? M3.color.onSurface : M3.color.surface
                        }
                    }
                }
            }

            Repeater {
                model: M3.typography.roleNames()

                delegate: StyledTextLabel {
                    required property var modelData

                    horizontalAlignment: Text.AlignLeft
                    text: modelData
                    font: M3.typography.font(modelData)
                    color: M3.color.onSurface
                }
            }
        }
    }
}
