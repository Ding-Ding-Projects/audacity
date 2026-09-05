/*
* Audacity: A Digital Audio Editor
*
* CommandPaletteHost
*
* The one item the application window has to carry for the command palette to
* exist. It owns the palette model, registers it as the handler for the
* command palette action so that the global Ctrl+Shift+F shortcut reaches it,
* and creates the palette surface itself lazily the first time it opens.
*
* It also performs the teleports the palette asks for: opening the preferences
* dialog on the right page with the right control highlighted, opening a
* documentation article, or raising a panel.
*
* API:
*     opened, open(), close()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.Companion

Item {
    id: root

    objectName: "CommandPaletteHost"

    anchors.fill: parent

    // The host itself must not eat mouse events while the palette is closed.
    visible: paletteLoader.active
    enabled: paletteLoader.active

    readonly property bool opened: paletteLoader.active && paletteLoader.item !== null && paletteLoader.item.opened

    function open() {
        paletteLoader.active = true
        if (paletteLoader.item !== null) {
            paletteLoader.item.open()
        }
    }

    // The window that owns this host knows its own pages and does not have to
    // wait for the palette to be open to report them: it can call this the
    // moment it is ready. Each row needs a title and a uri; subtitle and
    // section are optional.
    function setContextRows(rows) {
        companionModel.setContextRows(rows)
    }

    function close() {
        if (paletteLoader.item !== null) {
            paletteLoader.item.close()
        }
    }

    CommandPaletteModel {
        id: companionModel

        Component.onCompleted: {
            companionModel.registerAsPaletteHost()
        }

        onOpenRequested: {
            if (root.opened) {
                root.close()
            } else {
                root.open()
            }
        }

        onActivated: {
            root.close()
        }

        onTeleportToPreferences: function (pageId, target) {
            var uri = "audacity://preferences?currentPageId=" + encodeURIComponent(pageId)
            if (target !== "") {
                uri += "&highlight=" + encodeURIComponent(target)
            }
            api.launcher.open(uri)
        }

        onTeleportToDocument: function (path) {
            api.launcher.openUrl("file://" + path)
        }

        onTeleportToContext: function (payload) {
            if (Boolean(payload) && Boolean(payload.uri)) {
                api.launcher.open(payload.uri)
            }
        }
    }

    Loader {
        id: paletteLoader

        anchors.fill: parent
        active: false

        sourceComponent: CommandPalette {
            paletteModel: companionModel
            fullWindow: companionModel.fullWindow

            onClosed: {
                paletteLoader.active = false
            }
        }

        onLoaded: {
            paletteLoader.item.open()
        }
    }
}
