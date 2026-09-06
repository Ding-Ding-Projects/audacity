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

//! The queue stores one bounded JSON record per item. It streams pages from
//! disk and never retains the whole queue or any file bytes in memory.
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
    QString recordsPath() const;
};
}
