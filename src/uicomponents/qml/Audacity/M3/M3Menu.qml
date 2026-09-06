/*
* Audacity: A Digital Audio Editor
*
* M3Menu
*
* A Material 3 menu built on the muse StyledPopupView, so the application's
* popup stack, escape handling and navigation sections keep working. It draws
* a level 2 surface with extra small corners, supports separators, checkable
* items, right aligned shortcuts, submenus and an optional built in search
* field wired to the regular expression builder.
*
* Replaces: Muse.UiComponents StyledMenu.
*
* Every menu, short or long, shows the filter field: plain text search is
* the default, and its own regular expression builder is one click away.
* A host may listen for regexBuilderRequested to open a shared builder of
* its own, but nothing has to: this menu answers the request itself with a
* built in RegexBuilderSheet, so a menu never needs an external host.
*
* API:
*     model (list of { id, title, shortcut, icon, checkable, checked,
*            separator, subitems }), searchable, filterText,
*     handleMenuItem(id), regexBuilderRequested(), open(), close()
*
* The model may also be a muse MenuItemList. Those objects name the shortcut
* text "shortcuts" and mark a separator by carrying no title, so both
* spellings are accepted.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion

StyledPopupView {
    id: root

    property var model: []

    // Shows a search field at the top of the menu. Every menu is searchable
    // by default, not only the long ones: a short menu that stops being
    // short after a later change should not need this flag revisited.
    property bool searchable: true
    property string filterText: ""

    // A distinct identifier for this menu, used to keep its regular
    // expression builder state (pattern, flags, saved cases) separate from
    // every other menu and search field in the application.
    property string menuName: "M3Menu"

    property NavigationPanel navigationPanel: null

    signal handleMenuItem(string itemId)
    signal regexBuilderRequested

    // Personalize appearance override hookup, see M3Button.qml for detail.
    property string elementId: ""
    property int appearanceRevision: 0

    function m3Appearance(property, fallback) {
        root.appearanceRevision
        if (root.elementId === "" || typeof AppearanceOverrides === "undefined") {
            return fallback
        }
        return AppearanceOverrides.resolve(root.elementId, "", property, fallback)
    }

    Connections {
        target: typeof AppearanceOverrides !== "undefined" ? AppearanceOverrides : null
        ignoreUnknownSignals: true

        function onElementChanged(elementId) {
            if (elementId === root.elementId) {
                root.appearanceRevision = root.appearanceRevision + 1
            }
        }
    }

    // Items that survive the current filter text.
    readonly property var visibleItems: {
        if (!root.searchable || root.filterText === "") {
            return root.model
        }
        var needle = root.filterText.toLowerCase()
        var result = []
        for (var i = 0; i < root.model.length; ++i) {
            var item = root.model[i]
            if (item.separator === true || !item.title) {
                continue
            }
            var title = item.title !== undefined ? String(item.title) : ""
            if (title.toLowerCase().indexOf(needle) !== -1) {
                result.push(item)
            }
        }
        return result
    }

    cornerRadius: root.m3Appearance("radius", M3.shape.extraSmall)
    elevationLevel: root.m3Appearance("elevation", 2)
    backgroundColor: root.m3Appearance("containerColor", M3.surfaceAt(2))
    borderColor: M3.color.outlineVariant

    padding: 0
    margins: 0
    verticalMargins: 8

    contentWidth: Math.max(112, column.implicitWidth)
    contentHeight: column.implicitHeight

    onOpened: {
        if (root.searchable) {
            search.searchText = ""
        }
    }

    onClosed: menuRegexBuilder.close()

    Column {
        id: column

        width: root.contentWidth

        M3SearchBar {
            id: search
            objectName: "M3MenuSearch"

            width: parent.width - 16
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.searchable
            showRegexBuilder: true
            placeholder: qsTrc("uicomponents", "Filter")

            Accessible.description: qsTrc("uicomponents", "Filters the menu items below. Plain text by default, or use the regular expression builder.")

            Keys.onEscapePressed: function (event) {
                if (search.searchText !== "") {
                    search.clear()
                    root.filterText = ""
                    event.accepted = true
                } else {
                    root.close()
                    event.accepted = true
                }
            }

            onSearchTextChanged: root.filterText = search.searchText
            onRegexBuilderRequested: {
                menuRegexBuilder.pattern = search.searchText
                menuRegexBuilder.open()
                root.regexBuilderRequested()
            }
        }

        // The screen reader result count. Visually hidden but always
        // present, so a filtered menu never reports its count only to a
        // sighted user.
        Item {
            width: 1
            height: 1
            visible: false

            Accessible.role: Accessible.StaticText
            Accessible.name: root.searchable && root.filterText !== "" ? qsTrc("uicomponents", "%n result(s)", "", root.visibleItems.length) : ""
        }

        StyledTextLabel {
            width: parent.width - 16
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.searchable && root.filterText !== "" && root.visibleItems.length === 0
            horizontalAlignment: Text.AlignHCenter

            //: Shown in a menu's filter field when nothing matches the typed text.
            text: qsTrc("uicomponents", "No matching items")
        }

        Repeater {
            id: repeater

            model: root.visibleItems

            delegate: M3MenuItem {
                id: item

                required property int index
                required property var modelData

                width: column.width

                text: item.modelData.title !== undefined ? item.modelData.title : ""
                icon: item.modelData.icon !== undefined ? item.modelData.icon : IconCode.NONE
                shortcut: {
                    if (item.modelData.shortcut !== undefined) {
                        return String(item.modelData.shortcut)
                    }
                    // A muse MenuItem spells the shortcut text "shortcuts".
                    return item.modelData.shortcuts !== undefined ? String(item.modelData.shortcuts) : ""
                }
                checkable: item.modelData.checkable === true
                checked: item.modelData.checked === true
                enabled: item.modelData.enabled !== false
                isSeparator: item.modelData.separator === true || !item.modelData.title
                hasSubMenu: item.modelData.subitems !== undefined && item.modelData.subitems !== null && item.modelData.subitems.length > 0

                navigation.panel: root.navigationPanel
                navigation.row: item.index

                onTriggered: {
                    root.handleMenuItem(item.modelData.id !== undefined ? String(item.modelData.id) : "")
                    root.close()
                }

                onSubMenuRequested: {
                    subMenuLoader.active = true
                    if (subMenuLoader.item) {
                        // The loaded item is another M3Menu. Its type is only
                        // known at run time, so these members cannot be checked
                        // statically.
                        // qmllint disable missing-property
                        subMenuLoader.item.model = item.modelData.subitems
                        subMenuLoader.item.parent = item
                        subMenuLoader.item.open()
                        // qmllint enable missing-property
                    }
                }
            }
        }
    }

    /*
     * The default builder host. A menu never needs an outside surface to
     * answer regexBuilderRequested: this sheet opens right over the menu's
     * own content, keyed by this menu's own name, and never bleeds its
     * pattern, flags or saved cases into any other field's builder.
     */
    RegexBuilderSheet {
        id: menuRegexBuilder

        z: 10
        width: root.contentWidth
        height: root.contentHeight

        storeName: root.menuName
        fieldLabel: qsTrc("uicomponents", "Filter")

        onPatternAccepted: function (pattern) {
            search.searchText = pattern
            root.filterText = pattern
        }

        onClosed: search.forceActiveFocus()
    }

    /*
     * Submenus are loaded lazily and by file name. A QML component cannot
     * instantiate itself directly, and lazy loading also keeps a deep menu tree
     * from being built before it is opened.
     */
    Loader {
        id: subMenuLoader

        active: false
        source: "M3Menu.qml"

        onLoaded: {
            // qmllint disable missing-property
            subMenuLoader.item.navigationPanel = root.navigationPanel
            subMenuLoader.item.handleMenuItem.connect(function (itemId) {
                root.handleMenuItem(itemId)
                root.close()
            })
            // qmllint enable missing-property
        }
    }
}
