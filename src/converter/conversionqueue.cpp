/*
* Audacity: A Digital Audio Editor
*/

#include "conversionqueue.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

#include <utility>

using namespace au::converter;

namespace {
QString stateName(QueueItemState state)
{
    switch (state) {
    case QueueItemState::Pending: return QStringLiteral("pending");
    case QueueItemState::Running: return QStringLiteral("running");
    case QueueItemState::Cancelled: return QStringLiteral("cancelled");
    case QueueItemState::Converted: return QStringLiteral("converted");
    case QueueItemState::Failed: return QStringLiteral("failed");
    }
    return QStringLiteral("failed");
}

QueueItemState parseState(const QString& state)
{
    if (state == QStringLiteral("pending") || state == QStringLiteral("running")) return QueueItemState::Pending;
    if (state == QStringLiteral("cancelled")) return QueueItemState::Cancelled;
    if (state == QStringLiteral("converted")) return QueueItemState::Converted;
    return QueueItemState::Failed;
}
}

ConversionQueue::ConversionQueue(QString statePath)
    : m_statePath(std::move(statePath))
{
}

bool ConversionQueue::enqueue(const ConversionRequest& request, QString* error)
{
    if (request.sourcePath.isEmpty() || request.outputPath.isEmpty() || request.targetFormat.isEmpty()) {
        if (error) *error = QStringLiteral("A queue item needs source, destination and target format paths.");
        return false;
    }
    QueueItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.request = request;
    m_items.append(item);
    return save(error);
}

QVector<QueueItem> ConversionQueue::page(int offset, int limit) const
{
    QVector<QueueItem> result;
    if (offset < 0 || limit <= 0 || offset >= m_items.size()) return result;
    const int end = qMin(m_items.size(), offset + qMin(limit, 500));
    result.reserve(end - offset);
    for (int i = offset; i < end; ++i) result.append(m_items.at(i));
    return result;
}

int ConversionQueue::count() const { return m_items.size(); }

bool ConversionQueue::cancel(const QString& id)
{
    for (QueueItem& item : m_items) {
        if (item.id == id && (item.state == QueueItemState::Pending || item.state == QueueItemState::Running)) {
            item.state = QueueItemState::Cancelled;
            item.message = QStringLiteral("Cancelled by the user.");
            return save();
        }
    }
    return false;
}

bool ConversionQueue::load(QString* error)
{
    m_items.clear();
    QFile file(m_statePath);
    if (!file.exists()) return true;
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("The queue state file cannot be read.");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("The queue state is malformed.");
        return false;
    }
    const QJsonArray items = document.object().value(QStringLiteral("items")).toArray();
    for (const QJsonValue& value : items) {
        const QJsonObject object = value.toObject();
        QueueItem item;
        item.id = object.value(QStringLiteral("id")).toString();
        item.request.sourcePath = object.value(QStringLiteral("sourcePath")).toString();
        item.request.outputPath = object.value(QStringLiteral("outputPath")).toString();
        item.request.targetFormat = object.value(QStringLiteral("targetFormat")).toString();
        item.request.allowOverwrite = object.value(QStringLiteral("allowOverwrite")).toBool(false);
        item.state = parseState(object.value(QStringLiteral("state")).toString());
        item.message = object.value(QStringLiteral("message")).toString();
        if (!item.id.isEmpty() && !item.request.sourcePath.isEmpty() && !item.request.outputPath.isEmpty()) m_items.append(item);
    }
    return true;
}

bool ConversionQueue::save(QString* error) const
{
    QFileInfo info(m_statePath);
    QDir().mkpath(info.absolutePath());
    QJsonArray items;
    for (const QueueItem& item : m_items) {
        QJsonObject object;
        object[QStringLiteral("id")] = item.id;
        object[QStringLiteral("sourcePath")] = item.request.sourcePath;
        object[QStringLiteral("outputPath")] = item.request.outputPath;
        object[QStringLiteral("targetFormat")] = item.request.targetFormat;
        object[QStringLiteral("allowOverwrite")] = item.request.allowOverwrite;
        object[QStringLiteral("state")] = stateName(item.state);
        object[QStringLiteral("message")] = item.message;
        items.append(object);
    }
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("items")] = items;
    QSaveFile file(m_statePath);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(root).toJson(QJsonDocument::Compact)) < 0 || !file.commit()) {
        if (error) *error = QStringLiteral("The queue state could not be saved atomically.");
        return false;
    }
    return true;
}
