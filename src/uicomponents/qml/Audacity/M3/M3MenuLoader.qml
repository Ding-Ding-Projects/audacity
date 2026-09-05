/*
* Audacity: A Digital Audio Editor
*
* M3MenuLoader
*
* Lazily loads an M3Menu popup, matching the calling convention of the muse
* StyledMenuLoader so a top level menu bar can host Material 3 dropdown
* menus without pulling in the legacy Muse.UiComponents StyledMenu.
*
* Replaces: Muse.UiComponents StyledMenuLoader.
*
* API: open(model, x, y), toggleOpened(model, x, y), close(),
*      update(model, x, y), isMenuOpened, menu, menuAnchorItem,
*      handleMenuItem(itemId), opened(), closed(force)
*/
pragma ComponentBehavior: Bound

import QtQuick

import Audacity.M3

Loader {
    id: loader

    signal handleMenuItem(string itemId)
    signal opened
    signal closed(bool force)

    property Item menuAnchorItem: null
    property var navigationPanel: null
    property alias isMenuOpened: loader.active
    property M3Menu menu: loader.item as M3Menu

    active: false

    sourceComponent: M3Menu {
        id: itemMenu

        navigationPanel: loader.navigationPanel

        onHandleMenuItem: function (itemId) {
            itemMenu.close()
            Qt.callLater(loader.handleMenuItem, itemId)
        }

        onClosed: {
            Qt.callLater(loader.unload)
        }

        onOpened: {
            Qt.callLater(loader.opened)
        }
    }

    function unload() {
        loader.active = false
        Qt.callLater(loader.closed, false)
    }

    function open(model, x, y) {
        loader.active = true

        var menu = loader.menu
        menu.parent = loader.parent
        menu.anchorItem = loader.menuAnchorItem

        update(model, x, y)
        menu.open()
    }

    function toggleOpened(model, x, y) {
        loader.active = true

        if (loader.menu.isOpened) {
            loader.menu.close()
            return
        }

        open(model, x, y)
    }

    function close() {
        if (loader.isMenuOpened && loader.menu) {
            loader.menu.close()
        }
    }

    function update(model, x, y) {
        var menu = loader.menu
        if (!Boolean(menu)) {
            return
        }

        menu.model = model
        menu.x = x !== undefined && x !== -1 ? x : 0
        menu.y = y !== undefined && y !== -1 ? y : Qt.binding(function () { return menu.parent ? menu.parent.height : 0 })
    }
}
