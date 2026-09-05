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
    property var bookmarkedIds: []

    DocsIndex {
        id: docsIndex
    }

    function openArticle(id) {
        root.currentArticleId = id
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

        BulkSelectionController {
            Layout.fillWidth: true
            totalCount: articleRepeater.count
            destructiveActionLabel: qsTrc("toolkit", "Remove bookmark")
            onActionRequested: function (actionId, indexes) {
                // The host page owns which articles are bookmarked; this
                // only reports which rows were chosen.
            }
        }

        ListView {
            id: articleListView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            model: docsIndex.search(searchBar.searchText)

            delegate: M3ListItem {
                required property var modelData

                width: articleListView.width
                headline: modelData.title
                onClicked: root.openArticle(modelData.id)
            }
        }

        Repeater {
            id: articleRepeater

            model: docsIndex.articles.length

            delegate: Item {}
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
