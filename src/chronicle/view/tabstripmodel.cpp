/*
* Audacity: A Digital Audio Editor
*/
#include "tabstripmodel.h"

#include <QUuid>

#include "settings.h"

using namespace au::chronicle;

static const std::string MODULE_NAME("chronicle");
static const muse::Settings::Key SURFACES_KEY(MODULE_NAME, "chronicle/tabSurfaces");

static muse::Settings::Key stripKey(const QString& surfaceId)
{
    return muse::Settings::Key(MODULE_NAME, "chronicle/tabs/" + surfaceId.toStdString());
}

TabStripModel::TabStripModel(QObject* parent)
    : QObject(parent)
{
}

QStringList TabStripModel::storedSurfaceIds()
{
    const QString stored = QString::fromStdString(muse::settings()->value(SURFACES_KEY).toString());
    return stored.isEmpty() ? QStringList() : stored.split(QChar(u','), Qt::SkipEmptyParts);
}

void TabStripModel::setSurfaceId(const QString& value)
{
    if (m_state.surfaceId == value) {
        return;
    }
    m_state.surfaceId = value;
    emit surfaceIdChanged();
}

void TabStripModel::load()
{
    if (m_state.surfaceId.isEmpty()) {
        return;
    }

    const QString stored = QString::fromStdString(muse::settings()->value(stripKey(m_state.surfaceId)).toString());
    const QString surfaceId = m_state.surfaceId;
    m_state = TabStripLogic::deserialize(stored);
    m_state.surfaceId = surfaceId;
    m_restored = !stored.isEmpty();

    emit tabsChanged();
    emit dockSideChanged();
    emit collapsedChanged();
}

void TabStripModel::save()
{
    if (m_state.surfaceId.isEmpty()) {
        return;
    }

    muse::settings()->setSharedValue(stripKey(m_state.surfaceId),
                                     muse::Val(TabStripLogic::serialize(m_state).toStdString()));

    QStringList surfaces = storedSurfaceIds();
    if (!surfaces.contains(m_state.surfaceId)) {
        surfaces.append(m_state.surfaceId);
        muse::settings()->setSharedValue(SURFACES_KEY, muse::Val(surfaces.join(QChar(u',')).toStdString()));
    }
}

QVariantMap TabStripModel::tabToVariant(const TabItem& tab)
{
    QVariantMap item;
    item.insert(QStringLiteral("id"), tab.id);
    item.insert(QStringLiteral("title"), tab.title);
    item.insert(QStringLiteral("kind"), tab.kind);
    item.insert(QStringLiteral("uri"), tab.uri);
    item.insert(QStringLiteral("icon"), tab.icon);
    item.insert(QStringLiteral("pinned"), tab.pinned);
    item.insert(QStringLiteral("closable"), tab.closable);
    item.insert(QStringLiteral("groupId"), tab.groupId);
    return item;
}

QVariantList TabStripModel::tabs() const
{
    // Pinned tabs are shown first, and the stored order is kept within each
    // of the two runs.
    QVariantList pinned;
    QVariantList rest;
    for (const TabItem& tab : m_state.tabs) {
        (tab.pinned ? pinned : rest).append(tabToVariant(tab));
    }
    return pinned + rest;
}

QVariantList TabStripModel::groups() const
{
    QVariantList result;
    for (const TabGroup& group : m_state.groups) {
        int count = 0;
        for (const TabItem& tab : m_state.tabs) {
            if (tab.groupId == group.id) {
                ++count;
            }
        }
        QVariantMap item;
        item.insert(QStringLiteral("id"), group.id);
        item.insert(QStringLiteral("name"), group.name);
        item.insert(QStringLiteral("color"), group.color);
        item.insert(QStringLiteral("collapsed"), group.collapsed);
        item.insert(QStringLiteral("count"), count);
        result.append(item);
    }
    return result;
}

void TabStripModel::setDockSide(const QString& value)
{
    static const QStringList sides { QStringLiteral("left"), QStringLiteral("right"),
                                     QStringLiteral("top"), QStringLiteral("bottom") };
    const QString side = sides.contains(value) ? value : QStringLiteral("left");
    if (m_state.dockSide == side) {
        return;
    }
    m_state.dockSide = side;
    save();
    emit dockSideChanged();
}

bool TabStripModel::vertical() const
{
    return m_state.dockSide == QStringLiteral("left") || m_state.dockSide == QStringLiteral("right");
}

void TabStripModel::setCollapsed(bool value)
{
    if (m_state.collapsed == value) {
        return;
    }
    m_state.collapsed = value;
    save();
    emit collapsedChanged();
}

void TabStripModel::beginDeclare()
{
    m_declaring = true;
    m_declared.clear();
}

void TabStripModel::declareTab(const QString& id, const QString& title, const QString& kind,
                               const QString& uri, int icon, bool closable)
{
    if (id.isEmpty()) {
        return;
    }
    m_declared.append(id);

    for (int i = 0; i < m_state.tabs.size(); ++i) {
        if (m_state.tabs.at(i).id != id) {
            continue;
        }
        TabItem tab = m_state.tabs.at(i);
        tab.title = title;
        tab.kind = kind;
        tab.uri = uri;
        tab.icon = icon;
        tab.closable = closable;
        m_state.tabs.replace(i, tab);
        emit tabsChanged();
        return;
    }

    TabItem tab;
    tab.id = id;
    tab.title = title;
    tab.kind = kind;
    tab.uri = uri;
    tab.icon = icon;
    tab.closable = closable;
    m_state.tabs.append(tab);
    emit tabsChanged();
}

void TabStripModel::endDeclare()
{
    if (!m_declaring) {
        return;
    }
    m_declaring = false;

    QList<TabItem> kept;
    for (const TabItem& tab : m_state.tabs) {
        if (m_declared.contains(tab.id)) {
            kept.append(tab);
        }
    }
    m_state.tabs = kept;
    save();
    emit tabsChanged();
}

void TabStripModel::closeTab(const QString& id)
{
    for (int i = 0; i < m_state.tabs.size(); ++i) {
        if (m_state.tabs.at(i).id != id) {
            continue;
        }
        if (!m_state.tabs.at(i).closable) {
            return;
        }
        m_state.tabs.removeAt(i);
        save();
        emit tabsChanged();
        emit tabClosed(id);
        return;
    }
}

void TabStripModel::moveTab(int from, int to)
{
    if (from < 0 || from >= m_state.tabs.size() || to < 0 || to >= m_state.tabs.size() || from == to) {
        return;
    }
    m_state.tabs.move(from, to);
    save();
    emit tabsChanged();
}

void TabStripModel::setPinned(const QString& id, bool pinned)
{
    for (int i = 0; i < m_state.tabs.size(); ++i) {
        if (m_state.tabs.at(i).id != id) {
            continue;
        }
        TabItem tab = m_state.tabs.at(i);
        tab.pinned = pinned;
        m_state.tabs.replace(i, tab);
        save();
        emit tabsChanged();
        return;
    }
}

QString TabStripModel::createGroup(const QString& name, const QString& color)
{
    TabGroup group;
    group.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    group.name = name.isEmpty() ? QStringLiteral("New group") : name;
    if (!color.isEmpty()) {
        group.color = color;
    }
    m_state.groups.append(group);
    save();
    emit tabsChanged();
    return group.id;
}

void TabStripModel::setGroupAppearance(const QString& groupId, const QString& name, const QString& color)
{
    for (int i = 0; i < m_state.groups.size(); ++i) {
        if (m_state.groups.at(i).id != groupId) {
            continue;
        }
        TabGroup group = m_state.groups.at(i);
        if (!name.isEmpty()) {
            group.name = name;
        }
        if (!color.isEmpty()) {
            group.color = color;
        }
        m_state.groups.replace(i, group);
        save();
        emit tabsChanged();
        return;
    }
}

void TabStripModel::assignToGroup(const QString& tabId, const QString& groupId)
{
    for (int i = 0; i < m_state.tabs.size(); ++i) {
        if (m_state.tabs.at(i).id != tabId) {
            continue;
        }
        TabItem tab = m_state.tabs.at(i);
        tab.groupId = groupId;
        m_state.tabs.replace(i, tab);
        save();
        emit tabsChanged();
        return;
    }
}

void TabStripModel::removeGroup(const QString& groupId)
{
    for (int i = 0; i < m_state.groups.size(); ++i) {
        if (m_state.groups.at(i).id == groupId) {
            m_state.groups.removeAt(i);
            break;
        }
    }
    for (int i = 0; i < m_state.tabs.size(); ++i) {
        if (m_state.tabs.at(i).groupId != groupId) {
            continue;
        }
        TabItem tab = m_state.tabs.at(i);
        tab.groupId.clear();
        m_state.tabs.replace(i, tab);
    }
    save();
    emit tabsChanged();
}

QVariantList TabStripModel::searchStrip(const QString& query, bool useRegex) const
{
    QVariantList result;
    for (const TabItem& tab : m_state.tabs) {
        if (query.isEmpty() || TabStripLogic::matches(tab.title, query, useRegex)) {
            result.append(tabToVariant(tab));
        }
    }
    return result;
}

QVariantList TabStripModel::searchGroup(const QString& groupId, const QString& query, bool useRegex) const
{
    QVariantList result;
    for (const TabItem& tab : m_state.tabs) {
        if (tab.groupId != groupId) {
            continue;
        }
        if (query.isEmpty() || TabStripLogic::matches(tab.title, query, useRegex)) {
            result.append(tabToVariant(tab));
        }
    }
    return result;
}

QVariantList TabStripModel::searchGroups(const QString& query, bool useRegex) const
{
    QVariantList result;
    for (const QVariant& value : groups()) {
        const QVariantMap group = value.toMap();
        const QString name = group.value(QStringLiteral("name")).toString();
        if (query.isEmpty() || TabStripLogic::matches(name, query, useRegex)) {
            result.append(group);
        }
    }
    return result;
}

QVariantList TabStripModel::searchAllStrips(const QString& query, bool useRegex) const
{
    QVariantList result;
    for (const QString& surfaceId : storedSurfaceIds()) {
        const QString stored = QString::fromStdString(muse::settings()->value(stripKey(surfaceId)).toString());
        const TabStripState state = TabStripLogic::deserialize(stored);
        for (const TabItem& tab : state.tabs) {
            if (!query.isEmpty() && !TabStripLogic::matches(tab.title, query, useRegex)) {
                continue;
            }
            QVariantMap item = tabToVariant(tab);
            item.insert(QStringLiteral("surfaceId"), surfaceId);
            result.append(item);
        }
    }
    return result;
}

QVariantList TabStripModel::closePreview(const QString& query, bool useRegex, bool containing,
                                         bool includePinned) const
{
    QVariantList result;
    for (const TabItem& tab : TabStripLogic::tabsToClose(m_state, query, useRegex, containing, includePinned)) {
        result.append(tabToVariant(tab));
    }
    return result;
}

int TabStripModel::applyClose(const QString& query, bool useRegex, bool containing, bool includePinned)
{
    const QList<TabItem> victims = TabStripLogic::tabsToClose(m_state, query, useRegex, containing, includePinned);
    for (const TabItem& tab : victims) {
        closeTab(tab.id);
    }
    return victims.size();
}

bool TabStripModel::isValidRegex(const QString& query) const
{
    return TabStripLogic::isValidRegex(query);
}
