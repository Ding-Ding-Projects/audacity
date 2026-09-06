/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include "conversionengine.h"

#include <QVector>

namespace au::converter {

enum class QueueItemState { Pending, Running, Cancelled, Converted, Failed };

struct QueueItem {
    QString id;
    ConversionRequest request;
    QueueItemState state = QueueItemState::Pending;
    QString message;
};

//! The queue stores paths and bounded state only.  It never stores source or
//! output bytes and callers read pages, rather than forcing a long queue into
//! a view-model sized in-memory list.
class ConversionQueue
{
public:
    explicit ConversionQueue(QString statePath);

    bool enqueue(const ConversionRequest& request, QString* error = nullptr);
    QVector<QueueItem> page(int offset, int limit) const;
    int count() const;
    bool cancel(const QString& id);
    bool load(QString* error = nullptr);
    bool save(QString* error = nullptr) const;

private:
    QString m_statePath;
    QVector<QueueItem> m_items;
};
}
