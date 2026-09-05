/*
* Audacity: A Digital Audio Editor
*/
#include "tabstripstate.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

using namespace au::chronicle;

QJsonObject TabStripLogic::toJson(const TabStripState& state)
{
    QJsonArray tabs;
    for (const TabItem& tab : state.tabs) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), tab.id);
        object.insert(QStringLiteral("title"), tab.title);
        object.insert(QStringLiteral("kind"), tab.kind);
        object.insert(QStringLiteral("uri"), tab.uri);
        object.insert(QStringLiteral("icon"), tab.icon);
        object.insert(QStringLiteral("pinned"), tab.pinned);
        object.insert(QStringLiteral("closable"), tab.closable);
        object.insert(QStringLiteral("groupId"), tab.groupId);
        tabs.append(object);
    }

    QJsonArray groups;
    for (const TabGroup& group : state.groups) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), group.id);
        object.insert(QStringLiteral("name"), group.name);
        object.insert(QStringLiteral("color"), group.color);
        object.insert(QStringLiteral("collapsed"), group.collapsed);
        groups.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("surfaceId"), state.surfaceId);
    root.insert(QStringLiteral("dockSide"), state.dockSide);
    root.insert(QStringLiteral("collapsed"), state.collapsed);
    root.insert(QStringLiteral("tabs"), tabs);
    root.insert(QStringLiteral("groups"), groups);
    return root;
}

TabStripState TabStripLogic::fromJson(const QJsonObject& object)
{
    TabStripState state;
    state.surfaceId = object.value(QStringLiteral("surfaceId")).toString();

    const QString side = object.value(QStringLiteral("dockSide")).toString();
    static const QStringList sides { QStringLiteral("left"), QStringLiteral("right"),
                                     QStringLiteral("top"), QStringLiteral("bottom") };
    state.dockSide = sides.contains(side) ? side : QStringLiteral("left");

    state.collapsed = object.value(QStringLiteral("collapsed")).toBool(false);

    const QJsonArray tabs = object.value(QStringLiteral("tabs")).toArray();
    for (const QJsonValue& value : tabs) {
        const QJsonObject item = value.toObject();
        TabItem tab;
        tab.id = item.value(QStringLiteral("id")).toString();
        if (tab.id.isEmpty()) {
            continue;
        }
        tab.title = item.value(QStringLiteral("title")).toString();
        tab.kind = item.value(QStringLiteral("kind")).toString(QStringLiteral("page"));
        tab.uri = item.value(QStringLiteral("uri")).toString();
        tab.icon = item.value(QStringLiteral("icon")).toInt();
        tab.pinned = item.value(QStringLiteral("pinned")).toBool(false);
        tab.closable = item.value(QStringLiteral("closable")).toBool(true);
        tab.groupId = item.value(QStringLiteral("groupId")).toString();
        state.tabs.append(tab);
    }

    const QJsonArray groups = object.value(QStringLiteral("groups")).toArray();
    for (const QJsonValue& value : groups) {
        const QJsonObject item = value.toObject();
        TabGroup group;
        group.id = item.value(QStringLiteral("id")).toString();
        if (group.id.isEmpty()) {
            continue;
        }
        group.name = item.value(QStringLiteral("name")).toString();
        group.color = item.value(QStringLiteral("color")).toString(QStringLiteral("#926BFF"));
        group.collapsed = item.value(QStringLiteral("collapsed")).toBool(false);
        state.groups.append(group);
    }

    return state;
}

QString TabStripLogic::serialize(const TabStripState& state)
{
    return QString::fromUtf8(QJsonDocument(toJson(state)).toJson(QJsonDocument::Compact));
}

TabStripState TabStripLogic::deserialize(const QString& text)
{
    if (text.isEmpty()) {
        return TabStripState();
    }
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
    if (!document.isObject()) {
        return TabStripState();
    }
    return fromJson(document.object());
}

bool TabStripLogic::isValidRegex(const QString& query)
{
    if (query.isEmpty()) {
        return false;
    }
    return QRegularExpression(query).isValid();
}

bool TabStripLogic::matches(const QString& title, const QString& query, bool useRegex)
{
    if (query.isEmpty()) {
        return false;
    }

    if (useRegex) {
        const QRegularExpression expression(query, QRegularExpression::CaseInsensitiveOption);
        if (!expression.isValid()) {
            return false;
        }
        return expression.match(title).hasMatch();
    }

    return title.contains(query, Qt::CaseInsensitive);
}

QList<TabItem> TabStripLogic::tabsToClose(const TabStripState& state, const QString& query, bool useRegex,
                                          bool containing, bool includePinned)
{
    QList<TabItem> result;

    // An empty query, or a regular expression that does not compile, selects
    // nothing at all. Neither the command nor its inverse may act on one.
    if (query.isEmpty()) {
        return result;
    }
    if (useRegex && !isValidRegex(query)) {
        return result;
    }

    for (const TabItem& tab : state.tabs) {
        if (!tab.closable) {
            continue;
        }
        if (tab.pinned && !includePinned) {
            continue;
        }
        const bool hit = matches(tab.title, query, useRegex);
        if (hit == containing) {
            result.append(tab);
        }
    }

    return result;
}
