/*
* Audacity: A Digital Audio Editor
*
* DocsBrowserPage
*
* The in-app documentation browser. Every feature article under
* docs/features is bundled into this module's resources at build time and
* rendered here, with search over titles and bodies, article to article
* links resolved inside the browser, and suggested articles at the end of
* each one. A bulk selection bar lets bookmarked articles be exported or
* removed together.
*
* API:
*     none (self contained)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Toolkit
import Audacity.Companion

RowLayout {
    id: root

    property string currentArticleId: ""

    // Kept for source compatibility with anything that read the article
    // list's bookmark state directly; the actual bookmarks live in
    // BookmarkModel below.
    readonly property var bookmarkedIds: {
        var ids = []
        for (var i = 0; i < bookmarks.count; ++i) {
            ids.push(bookmarks.data(bookmarks.index(i, 0), BookmarkModel.ArticleIdRole))
        }
        return ids
    }

    DocsIndex {
        id: docsIndex
    }

    BookmarkModel {
        id: bookmarks
    }

    ExportSheet {
        id: exportSheet

        rows: bookmarks.toExportRows()
    }

    function openArticle(id) {
        root.currentArticleId = id
    }

    function isBookmarked(articleId) {
        return bookmarks.isBookmarked(articleId)
    }

    function toggleBookmark(articleId, title) {
        bookmarks.toggle(articleId, title)
    }

    ColumnLayout {
        Layout.preferredWidth: 260
        Layout.fillHeight: true
        spacing: 8

        M3SearchBar {
            id: searchBar

            Layout.fillWidth: true
            placeholder: qsTrc("toolkit", "Search articles")
            showRegexBuilder: true
            objectName: "DocsBrowserSearch"
            onRegexBuilderRequested: regexSheet.open()
        }

        RegexBuilderSheet {
            id: regexSheet

            anchors.fill: parent
            storeName: "toolkit-docs"
            fieldLabel: qsTrc("toolkit", "Documentation search")
            onPatternAccepted: function (pattern) {
                searchBar.searchText = pattern
            }
        }

        ListView {
            id: articleListView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            model: docsIndex.search(searchBar.searchText)

            delegate: M3ListItem {
                id: articleDelegate

                required property var modelData

                width: articleListView.width
                headline: modelData.title
                onClicked: root.openArticle(modelData.id)

                trailingContent: Component {
                    M3IconButton {
                        variant: root.isBookmarked(articleDelegate.modelData.id) ? "filled" : "standard"
                        icon: IconCode.STAR
                        accessibleName: root.isBookmarked(articleDelegate.modelData.id) ? qsTrc("toolkit", "Remove bookmark") : qsTrc("toolkit", "Add bookmark")
                        onClicked: root.toggleBookmark(articleDelegate.modelData.id, articleDelegate.modelData.title)
                    }
                }
            }
        }

        StyledTextLabel {
            visible: bookmarks.count > 0
            text: qsTrc("toolkit", "Bookmarks (%1)").arg(bookmarks.count)
            font: M3.typography.titleMedium
        }

        BulkSelectionController {
            visible: bookmarks.count > 0
            Layout.fillWidth: true
            totalCount: bookmarks.count
            destructiveActionLabel: qsTrc("toolkit", "Remove bookmark")
            onActionRequested: function (actionId, indexes) {
                if (actionId === "export") {
                    exportSheet.open()
                } else if (actionId === "destructive") {
                    bookmarks.removeMany(indexes)
                }
            }
        }

        ListView {
            id: bookmarkListView

            visible: bookmarks.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(bookmarks.count * 56, 168)
            clip: true

            model: bookmarks

            delegate: M3ListItem {
                required property string title
                required property string articleId

                width: bookmarkListView.width
                headline: title
                onClicked: root.openArticle(articleId)
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 12

        StyledTextLabel {
            text: docsIndex.articleById(root.currentArticleId).title || qsTrc("toolkit", "Choose an article")
            font: M3.typography.headlineSmall
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: articleBody.implicitHeight
            clip: true

            TextEdit {
                id: articleBody

                width: parent.width
                readOnly: true
                textFormat: TextEdit.MarkdownText
                text: docsIndex.articleById(root.currentArticleId).body || ""
                wrapMode: TextEdit.WordWrap
                selectByMouse: true

                onLinkActivated: function (link) {
                    // Article to article links use a bare article id, so
                    // clicking one resolves inside the browser instead of
                    // opening a system browser.
                    root.openArticle(link)
                }
            }
        }

        StyledTextLabel {
            visible: root.currentArticleId.length > 0
            text: qsTrc("toolkit", "Suggested articles")
            font: M3.typography.titleMedium
        }

        Repeater {
            model: root.currentArticleId.length > 0 ? docsIndex.suggestedArticles(root.currentArticleId, 3) : []

            delegate: M3Button {
                required property var modelData

                text: modelData.title
                variant: "text"
                onClicked: root.openArticle(modelData.id)
            }
        }
    }
}
