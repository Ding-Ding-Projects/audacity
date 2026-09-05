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

StyledPopupView {
    id: root

    property var model: []

    // Shows a search field at the top of the menu.
    property bool searchable: false
    property string filterText: ""

    property NavigationPanel navigationPanel: null

    // Exposed so a parent menu bar can find the deepest open submenu when
    // computing the area currently covered by an open menu tree.
    property alias subMenuLoader: subMenuLoader

    signal handleMenuItem(string itemId)
    signal regexBuilderRequested

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

    cornerRadius: M3.shape.extraSmall
    elevationLevel: 2
    backgroundColor: M3.surfaceAt(2)
    borderColor: M3.color.outlineVariant

    padding: 0
    margins: 0
    verticalMargins: 8

    contentWidth: Math.max(112, column.implicitWidth)
    contentHeight: column.implicitHeight

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
            placeholder: "Filter"

            onSearchTextChanged: root.filterText = search.searchText
            onRegexBuilderRequested: root.regexBuilderRequested()
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
     * Submenus are loaded lazily and by file name. A QML component cannot
     * instantiate itself directly, and lazy loading also keeps a deep menu tree
     * from being built before it is opened.
     */
    Loader {
        id: subMenuLoader

        // Compatibility aliases so a hosting menu bar can walk a chain of
        // open menus the same way it would with the muse StyledMenuLoader.
        property alias isMenuOpened: subMenuLoader.active
        property alias menu: subMenuLoader.item

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
