/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace au::toolkit {
//! The batch pull cart: a list of models scheduled for a local pull. This
//! is deliberately free of any pricing, payment, checkout, account or
//! subscription concept, because adding a model here only ever schedules a
//! download through the local Ollama installation; nothing is purchased.
class PullCartModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY itemsChanged)

public:
    explicit PullCartModel(QObject* parent = nullptr);

    QVariantList items() const;
    qint64 totalBytes() const;

    Q_INVOKABLE void addModel(const QString& tag, qint64 estimatedBytes);
    Q_INVOKABLE void removeModel(const QString& tag);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool contains(const QString& tag) const;

signals:
    void itemsChanged();

private:
    QVariantList m_items;
};
}
