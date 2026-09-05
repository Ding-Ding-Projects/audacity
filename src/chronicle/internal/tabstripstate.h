/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace au::chronicle {
//! One tab in a strip. A tab stands for an open project, a dockable panel or
//! one of the fixed application pages.
struct TabItem {
    QString id;
    QString title;
    //! "page", "project" or "panel".
    QString kind = QStringLiteral("page");
    QString uri;
    int icon = 0;
    bool pinned = false;
    bool closable = true;
    QString groupId;
};

//! A named, coloured group of tabs.
struct TabGroup {
    QString id;
    QString name;
    //! A colour string or the sentinel "rainbow", matching M3ColorPicker.
    QString color = QStringLiteral("#926BFF");
    bool collapsed = false;
};

//! Everything one strip persists across restarts.
struct TabStripState {
    QString surfaceId;
    //! "left", "right", "top" or "bottom". Left is the default.
    QString dockSide = QStringLiteral("left");
    //! Whether the strip is showing icons only.
    bool collapsed = false;
    QList<TabItem> tabs;
    QList<TabGroup> groups;
};

/*!
 * The pure logic behind the tab strip: what is persisted, how a search term
 * matches and which tabs a bulk close would take. It holds no Qt Quick and no
 * model, so it can be asserted directly.
 */
class TabStripLogic
{
public:
    static QJsonObject toJson(const TabStripState& state);
    static TabStripState fromJson(const QJsonObject& object);

    static QString serialize(const TabStripState& state);
    static TabStripState deserialize(const QString& text);

    //! True when the query is a usable regular expression.
    static bool isValidRegex(const QString& query);

    //! Matches a tab title against a query. Plain text is the default; a
    //! regular expression is used only when useRegex is set and the pattern
    //! compiles. Matching is case insensitive either way.
    static bool matches(const QString& title, const QString& query, bool useRegex);

    /*!
     * The tabs a bulk close would take.
     *
     * containing selects "close tabs containing text"; clearing it selects the
     * inverse, "close tabs not containing text". Pinned tabs are excluded
     * unless includePinned is set. An empty query and an invalid regular
     * expression both return nothing, so the command can never close
     * everything by accident.
     */
    static QList<TabItem> tabsToClose(const TabStripState& state, const QString& query, bool useRegex, bool containing,
                                      bool includePinned = false);
};
}
