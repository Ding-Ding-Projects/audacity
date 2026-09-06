/*
* Audacity: A Digital Audio Editor
*
* TabStrip
*
* The browser style tab strip. It hosts the fixed application pages, one tab
* per open project and one tab per dockable panel.
*
* The strip can be docked to the left, the right, the top or the bottom. The
* side is persisted per surface and can be changed from the strip's context
* menu and from Preferences. Changing the side changes the direction the tabs
* run in, never the direction the labels read in.
*
* API:
*     surfaceId, currentTabId, sources, tabActivated(id, uri),
*     regexBuilderRequested(context), model
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Chronicle

FocusScope {
    id: root

    // Identifies the strip in the stored preferences.
    property string surfaceId: "main"

    // The tabs the application currently offers, as a list of
    // { id, title, kind, uri, icon, closable }.
    property var sources: []

    property string currentTabId: ""

    // The width below which the strip shows icons only. A horizontal strip
    // compares its width, a vertical strip its own width.
    property real collapseThreshold: 420

    // The side this strip starts on when the user has never chosen one. Left
    // is the contract default; a host whose surface is a horizontal bar sets
    // "top" instead.
    property string defaultDockSide: "left"

    readonly property alias model: tabModel
    property alias navigationPanel: navPanel

    signal tabActivated(string id, string uri)
    signal tabCloseRequested(string id)
    // The context names which of the strip's searches asked for the builder,
    // so the host can attach the same builder to each of them.
    signal regexBuilderRequested(string context)

    // A horizontal strip's natural width has to come from its own tab count,
    // never from the ListView inside it: that ListView fills whatever width
    // it is handed, so reading its size back would be circular and a dock
    // host that sizes this item from its implicit width got zero every time,
    // which is why the strip used to render nothing but the overflow button.
    //
    // This always assumes labelled tabs, on purpose, even though the strip
    // may end up narrower than that and fall back to icons: the implicit
    // size is what the strip would like to be, not a guess that folds the
    // icons-only collapse back into its own input. Basing it on the
    // icons-only width instead created a self-fulfilling narrow strip: a
    // small width made tooNarrow true, which made implicitWidth small,
    // which kept the strip narrow forever, even once every tab, and the
    // room for their labels, was available.
    readonly property int tabCount: tabModel.tabs.length
    implicitWidth: tabModel.vertical ? (tabModel.collapsed ? M3.density.apply(64) : M3.density.apply(240)) : Math.max(M3.density.apply(160), root.tabCount * M3.density.apply(140) + M3.density.apply(56))
    implicitHeight: tabModel.vertical ? 0 : M3.density.apply(48)

    TabStripModel {
        id: tabModel

        surfaceId: root.surfaceId
    }

    QtObject {
        id: prv

        property string contextTabId: ""
        property string contextGroupId: ""

        // A narrow strip shows icons only. The user's own choice, made from
        // the context menu, is kept in the model and wins until the strip
        // becomes too narrow for labels.
        readonly property bool tooNarrow: tabModel.vertical ? root.width < M3.density.apply(140) : root.width < root.collapseThreshold

        readonly property bool iconsOnly: tabModel.collapsed || prv.tooNarrow

        function groupOf(groupId) {
            var groups = tabModel.groups
            for (var i = 0; i < groups.length; ++i) {
                if (groups[i].id === groupId) {
                    return groups[i]
                }
            }
            return null
        }

        function colorOf(groupId) {
            var group = prv.groupOf(groupId)
            if (!group) {
                return "transparent"
            }
            return group.color === "rainbow" ? M3.color.tertiary : group.color
        }
    }

    function reload() {
        tabModel.beginDeclare()
        for (var i = 0; i < root.sources.length; ++i) {
            var source = root.sources[i]
            tabModel.declareTab(String(source.id), String(source.title), source.kind !== undefined ? String(source.kind) : "page", source.uri !== undefined ? String(source.uri) : "", source.icon !== undefined ? source.icon : IconCode.NONE, source.closable === true)
        }
        tabModel.endDeclare()
    }

    // root.sources is bound from the host's own tabSources computation,
    // which can legitimately update its value more than once while the
    // component tree is still being built, before Component.onCompleted
    // has had a chance to run. Calling reload() that early declares tabs
    // and, through declareTab()/endDeclare(), saves the model's state to
    // disk while it is still holding its raw un-loaded default (dockSide
    // "left"). That premature save then makes the real tabModel.load()
    // call a few moments later see non-empty stored data and believe the
    // side was already restored, so the host's chosen defaultDockSide
    // ("top" for a horizontal toolbar strip) was silently never applied
    // and every strip stayed vertical. Ignore source changes until the
    // model has actually loaded once.
    property bool modelReady: false

    onSourcesChanged: {
        if (root.modelReady) {
            root.reload()
        }
    }

    Component.onCompleted: {
        tabModel.load()
        if (!tabModel.isRestored()) {
            tabModel.dockSide = root.defaultDockSide
        }
        root.modelReady = true
        root.reload()
    }

    NavigationPanel {
        id: navPanel

        name: "TabStrip." + root.surfaceId
        enabled: root.enabled && root.visible
        // Roving focus: the arrow keys move along the strip and only the
        // active tab is a tab stop.
        direction: tabModel.vertical ? NavigationPanel.Vertical : NavigationPanel.Horizontal

        accessible.name: qsTrc("chronicle", "Tab list") + " " + navPanel.directionInfo
    }

    Rectangle {
        anchors.fill: parent
        color: M3.color.surface
    }

    // The strip itself. A vertical strip is a column of tabs and a horizontal
    // strip a row; the labels read left to right in both.
    GridLayout {
        id: layout

        anchors.fill: parent
        anchors.margins: 4

        rows: tabModel.vertical ? 2 : 1
        columns: tabModel.vertical ? 1 : 2
        rowSpacing: 4
        columnSpacing: 4

        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            orientation: tabModel.vertical ? ListView.Vertical : ListView.Horizontal
            spacing: 2
            clip: true
            interactive: true

            model: tabModel.tabs

            delegate: TabStripItem {
                id: tabItem

                required property int index
                required property var modelData

                width: tabModel.vertical ? listView.width : implicitWidth
                height: tabModel.vertical ? M3.density.apply(40) : listView.height

                tabId: tabItem.modelData.id
                text: tabItem.modelData.title
                icon: tabItem.modelData.icon
                pinned: tabItem.modelData.pinned === true
                closable: tabItem.modelData.closable === true
                groupColor: prv.colorOf(tabItem.modelData.groupId)
                iconsOnly: prv.iconsOnly
                selected: tabItem.modelData.id === root.currentTabId

                navigation.panel: navPanel
                navigation.row: tabModel.vertical ? tabItem.index : 0
                navigation.column: tabModel.vertical ? 0 : tabItem.index

                onClicked: root.tabActivated(tabItem.modelData.id, tabItem.modelData.uri)
                onCloseRequested: {
                    tabModel.closeTab(tabItem.modelData.id)
                    root.tabCloseRequested(tabItem.modelData.id)
                }
                onContextMenuRequested: {
                    prv.contextTabId = tabItem.modelData.id
                    prv.contextGroupId = tabItem.modelData.groupId
                    contextMenu.parent = tabItem
                    contextMenu.open()
                }
                onMoveRequested: function (delta) {
                    tabModel.moveTab(tabItem.index, tabItem.index + delta)
                }
            }
        }

        // The overflow and search entry point. It is a popup window, so it is
        // never clipped by the strip, however narrow the strip becomes.
        M3IconButton {
            Layout.alignment: Qt.AlignCenter

            icon: IconCode.MENU_THREE_DOTS
            accessibleName: qsTrc("chronicle", "All tabs and tab actions")
            toolTipTitle: qsTrc("chronicle", "All tabs")

            navigation.panel: navPanel
            navigation.row: tabModel.vertical ? 9999 : 0
            navigation.column: tabModel.vertical ? 0 : 9999

            onClicked: {
                overflowMenu.parent = this
                overflowMenu.open()
            }
        }
    }

    M3Menu {
        id: contextMenu

        model: [
            {
                "id": "pin",
                "title": qsTrc("chronicle", "Pin tab")
            },
            {
                "id": "unpin",
                "title": qsTrc("chronicle", "Unpin tab")
            },
            {
                "separator": true
            },
            {
                "id": "new-group",
                "title": qsTrc("chronicle", "Add to a new group")
            },
            {
                "id": "ungroup",
                "title": qsTrc("chronicle", "Remove from group")
            },
            {
                "id": "group-appearance",
                "title": qsTrc("chronicle", "Edit group appearance…")
            },
            {
                "separator": true
            },
            {
                "id": "dock-left",
                "title": qsTrc("chronicle", "Dock the strip to the left")
            },
            {
                "id": "dock-right",
                "title": qsTrc("chronicle", "Dock the strip to the right")
            },
            {
                "id": "dock-top",
                "title": qsTrc("chronicle", "Dock the strip to the top")
            },
            {
                "id": "dock-bottom",
                "title": qsTrc("chronicle", "Dock the strip to the bottom")
            },
            {
                "separator": true
            },
            {
                "id": "collapse",
                "title": qsTrc("chronicle", "Show icons only")
            },
            {
                "id": "expand",
                "title": qsTrc("chronicle", "Show labels")
            },
            {
                "separator": true
            },
            {
                "id": "close-containing",
                "title": qsTrc("chronicle", "Close tabs containing text…")
            },
            {
                "id": "close-not-containing",
                "title": qsTrc("chronicle", "Close tabs not containing text…")
            }
        ]

        onHandleMenuItem: function (itemId) {
            switch (itemId) {
            case "pin":
                tabModel.setPinned(prv.contextTabId, true)
                break
            case "unpin":
                tabModel.setPinned(prv.contextTabId, false)
                break
            case "new-group":
                prv.contextGroupId = tabModel.createGroup(qsTrc("chronicle", "New group"), "#926BFF")
                tabModel.assignToGroup(prv.contextTabId, prv.contextGroupId)
                break
            case "ungroup":
                tabModel.assignToGroup(prv.contextTabId, "")
                break
            case "group-appearance":
                if (prv.contextGroupId !== "") {
                    groupAppearance.groupId = prv.contextGroupId
                    groupAppearance.parent = root
                    groupAppearance.open()
                }
                break
            case "dock-left":
                tabModel.dockSide = "left"
                break
            case "dock-right":
                tabModel.dockSide = "right"
                break
            case "dock-top":
                tabModel.dockSide = "top"
                break
            case "dock-bottom":
                tabModel.dockSide = "bottom"
                break
            case "collapse":
                tabModel.collapsed = true
                break
            case "expand":
                tabModel.collapsed = false
                break
            case "close-containing":
                closeTabs.containing = true
                closeTabs.parent = root
                closeTabs.open()
                break
            case "close-not-containing":
                closeTabs.containing = false
                closeTabs.parent = root
                closeTabs.open()
                break
            }
        }
    }

    TabSearchPopup {
        id: overflowMenu

        tabModel: tabModel

        onTabChosen: function (id, uri) {
            root.tabActivated(id, uri)
            overflowMenu.close()
        }

        onRegexBuilderRequested: function (context) {
            root.regexBuilderRequested(context)
        }

        onEditGroupRequested: function (groupId) {
            overflowMenu.close()
            groupAppearance.groupId = groupId
            groupAppearance.parent = root
            groupAppearance.open()
        }
    }

    TabGroupAppearancePopup {
        id: groupAppearance

        tabModel: tabModel
    }

    CloseTabsPopup {
        id: closeTabs

        tabModel: tabModel

        onRegexBuilderRequested: root.regexBuilderRequested("close-tabs")
    }
}
