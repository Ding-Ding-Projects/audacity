/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QVariantMap>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"

#include "iexperienceconfiguration.h"

namespace au::experience {
//! The scheduled settings table, as a list for the Material 3 list in the
//! preferences page.
class ScheduleListModel : public QAbstractListModel, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

    muse::GlobalInject<IExperienceConfiguration> configuration;

public:
    explicit ScheduleListModel(QObject* parent = nullptr);

    enum Roles {
        IdRole = Qt::UserRole + 1,
        EnabledRole,
        HourRole,
        MinuteRole,
        WeekdayMaskRole,
        KeyRole,
        ValueRole,
        TimeTextRole,
        DaysTextRole,
        SettingTextRole,
        NextFireTextRole
    };

    Q_INVOKABLE void load();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    //! Adds a row, or replaces the row with the same id. Returns the id, or an
    //! empty string when the row is not valid.
    Q_INVOKABLE QString save(const QVariantMap& row);
    Q_INVOKABLE void remove(const QString& id);
    Q_INVOKABLE void setEnabled(const QString& id, bool enabled);
    Q_INVOKABLE QVariantMap row(const QString& id) const;

    //! The settings a row may change, with their titles and value choices.
    Q_INVOKABLE QVariantList availableSettings() const;

signals:
    void countChanged();

private:
    void reload();
    QString settingText(const ScheduleEntry& entry) const;

    std::vector<ScheduleEntry> m_entries;
};
}
