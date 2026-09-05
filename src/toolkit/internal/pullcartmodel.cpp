/*
* Audacity: A Digital Audio Editor
*/

#include "pullcartmodel.h"

using namespace au::toolkit;

PullCartModel::PullCartModel(QObject* parent)
    : QObject(parent)
{
}

QVariantList PullCartModel::items() const
{
    return m_items;
}

qint64 PullCartModel::totalBytes() const
{
    qint64 total = 0;
    for (const QVariant& v : m_items) {
        total += v.toMap().value(QStringLiteral("estimatedBytes")).toLongLong();
    }
    return total;
}

void PullCartModel::addModel(const QString& tag, qint64 estimatedBytes)
{
    if (contains(tag)) {
        return;
    }
    QVariantMap item;
    item[QStringLiteral("tag")] = tag;
    item[QStringLiteral("estimatedBytes")] = estimatedBytes;
    item[QStringLiteral("state")] = QStringLiteral("queued");
    m_items << item;
    emit itemsChanged();
}

void PullCartModel::removeModel(const QString& tag)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).toMap().value(QStringLiteral("tag")).toString() == tag) {
            m_items.removeAt(i);
            emit itemsChanged();
            return;
        }
    }
}

void PullCartModel::clear()
{
    m_items.clear();
    emit itemsChanged();
}

bool PullCartModel::contains(const QString& tag) const
{
    for (const QVariant& v : m_items) {
        if (v.toMap().value(QStringLiteral("tag")).toString() == tag) {
            return true;
        }
    }
    return false;
}
