/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>
#include <QVariantList>

#include "internal/tabstripstate.h"

namespace au::chronicle {
/*!
 * The model behind one browser style tab strip.
 *
 * The strip hosts the fixed application pages, one tab per open project and
 * one tab per dockable panel. Order, pinning, groups, the dock side and the
 * collapsed state are persisted per surface, so a restart brings the strip
 * back exactly as it was left.
 *
 * All the persistence and search logic lives in TabStripLogic, which holds no
 * Qt Quick, so the behaviour is asserted directly rather than through the
 * interface.
 */
class TabStripModel : public QObject
{
    Q_OBJECT

    //! Identifies the strip in the stored preferences. Set before load().
    Q_PROPERTY(QString surfaceId READ surfaceId WRITE setSurfaceId NOTIFY surfaceIdChanged)

    Q_PROPERTY(QVariantList tabs READ tabs NOTIFY tabsChanged)
    Q_PROPERTY(QVariantList groups READ groups NOTIFY tabsChanged)

    //! "left", "right", "top" or "bottom".
    Q_PROPERTY(QString dockSide READ dockSide WRITE setDockSide NOTIFY dockSideChanged)
    Q_PROPERTY(bool collapsed READ collapsed WRITE setCollapsed NOTIFY collapsedChanged)
    Q_PROPERTY(bool vertical READ vertical NOTIFY dockSideChanged)

public:
    explicit TabStripModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    //! True when load() found stored state for this surface. A caller uses it
    //! to apply its own default dock side only on a first run.
    Q_INVOKABLE bool isRestored() const { return m_restored; }
    Q_INVOKABLE void save();

    QString surfaceId() const { return m_state.surfaceId; }
    void setSurfaceId(const QString& value);

    QVariantList tabs() const;
    QVariantList groups() const;

    QString dockSide() const { return m_state.dockSide; }
    void setDockSide(const QString& value);
    //! A left or right strip runs down the window, a top or bottom strip
    //! across it. The labels never rotate either way.
    bool vertical() const;

    bool collapsed() const { return m_state.collapsed; }
    void setCollapsed(bool value);

    //! Declares a tab. Existing tabs keep the order, pinning and group they
    //! were restored with; new ones are appended.
    Q_INVOKABLE void declareTab(const QString& id, const QString& title, const QString& kind, const QString& uri, int icon, bool closable);
    //! Removes every tab of the given kind that was not declared since the
    //! last call to beginDeclare(). Fixed pages are never removed.
    Q_INVOKABLE void beginDeclare();
    Q_INVOKABLE void endDeclare();

    Q_INVOKABLE void closeTab(const QString& id);
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void setPinned(const QString& id, bool pinned);

    Q_INVOKABLE QString createGroup(const QString& name, const QString& color);
    Q_INVOKABLE void setGroupAppearance(const QString& groupId, const QString& name, const QString& color);
    Q_INVOKABLE void assignToGroup(const QString& tabId, const QString& groupId);
    Q_INVOKABLE void removeGroup(const QString& groupId);

    //! The four searches the strip offers. Each returns the matching tabs.
    Q_INVOKABLE QVariantList searchStrip(const QString& query, bool useRegex) const;
    Q_INVOKABLE QVariantList searchGroup(const QString& groupId, const QString& query, bool useRegex) const;
    Q_INVOKABLE QVariantList searchGroups(const QString& query, bool useRegex) const;
    //! The master search covers every stored strip, not only this one.
    Q_INVOKABLE QVariantList searchAllStrips(const QString& query, bool useRegex) const;

    //! The tabs a bulk close would take, for the preview count.
    Q_INVOKABLE QVariantList closePreview(const QString& query, bool useRegex, bool containing, bool includePinned) const;
    //! Applies the bulk close and returns how many tabs were closed.
    Q_INVOKABLE int applyClose(const QString& query, bool useRegex, bool containing, bool includePinned);

    Q_INVOKABLE bool isValidRegex(const QString& query) const;

signals:
    void surfaceIdChanged();
    void tabsChanged();
    void dockSideChanged();
    void collapsedChanged();
    void tabClosed(QString id);

private:
    static QVariantMap tabToVariant(const TabItem& tab);
    static QStringList storedSurfaceIds();

    TabStripState m_state;
    QStringList m_declared;
    bool m_declaring = false;
    bool m_restored = false;
};
}
