/*
* Audacity: A Digital Audio Editor
*/

#include "conversionqueue.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

#include <utility>

using namespace au::converter;

namespace {
constexpr qint64 MaxRecordBytes = 64 * 1024;
constexpr int MaxPathChars = 16 * 1024;
constexpr int MaxMessageChars = 4 * 1024;

QString stateName(QueueItemState state)
{
    switch (state) {
    case QueueItemState::Pending: return QStringLiteral("pending");
    case QueueItemState::Running: return QStringLiteral("running");
    case QueueItemState::Cancelled: return QStringLiteral("cancelled");
    case QueueItemState::Converted: return QStringLiteral("converted");
    case QueueItemState::Failed: return QStringLiteral("failed");
    }
    return {};
}

bool parseState(const QString& text, QueueItemState* state)
{
    if (text == QStringLiteral("pending") || text == QStringLiteral("running")) *state = QueueItemState::Pending;
    else if (text == QStringLiteral("cancelled")) *state = QueueItemState::Cancelled;
    else if (text == QStringLiteral("converted")) *state = QueueItemState::Converted;
    else if (text == QStringLiteral("failed")) *state = QueueItemState::Failed;
    else return false;
    return true;
}

QJsonObject serialize(const QueueItem& item)
{
    return { { QStringLiteral("version"), 1 }, { QStringLiteral("id"), item.id },
             { QStringLiteral("sourcePath"), item.request.sourcePath }, { QStringLiteral("outputPath"), item.request.outputPath },
             { QStringLiteral("targetFormat"), item.request.targetFormat }, { QStringLiteral("allowOverwrite"), item.request.allowOverwrite },
             { QStringLiteral("state"), stateName(item.state) }, { QStringLiteral("message"), item.message } };
}

bool writeRecord(const QString& path, const QueueItem& item, QString* error)
{
    const QByteArray bytes = QJsonDocument(serialize(item)).toJson(QJsonDocument::Compact);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = QStringLiteral("The queue record could not be saved atomically.");
        return false;
    }
    return true;
}

bool readRecord(const QString& path, QueueItem* item, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > MaxRecordBytes) {
        if (error) *error = QStringLiteral("A queue record is unreadable or exceeds its size limit.");
        return false;
    }
    const QByteArray bytes = file.read(MaxRecordBytes + 1);
    const QList<QByteArray> keys { "version", "id", "sourcePath", "outputPath", "targetFormat", "allowOverwrite", "state", "message" };
    for (const QByteArray& key : keys) if (bytes.count(QByteArray("\"") + key + "\"") != 1) {
        if (error) *error = QStringLiteral("A queue record has duplicate or missing fields.");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    const QJsonObject object = document.isObject() ? document.object() : QJsonObject();
    const auto boundedString = [&object](const QString& key, int max) { return object.value(key).isString() && object.value(key).toString().size() <= max; };
    if (parseError.error != QJsonParseError::NoError || object.size() != keys.size() || object.value(QStringLiteral("version")).toInt(-1) != 1
        || !boundedString(QStringLiteral("id"), 64) || !boundedString(QStringLiteral("sourcePath"), MaxPathChars)
        || !boundedString(QStringLiteral("outputPath"), MaxPathChars) || !boundedString(QStringLiteral("targetFormat"), 32)
        || !object.value(QStringLiteral("allowOverwrite")).isBool() || !boundedString(QStringLiteral("state"), 16)
        || !boundedString(QStringLiteral("message"), MaxMessageChars)) {
        if (error) *error = QStringLiteral("A queue record failed schema validation.");
        return false;
    }
    QueueItem candidate;
    candidate.id = object.value(QStringLiteral("id")).toString();
    if (QUuid(candidate.id).isNull() || !parseState(object.value(QStringLiteral("state")).toString(), &candidate.state)) {
        if (error) *error = QStringLiteral("A queue record has an invalid identifier or state.");
        return false;
    }
    candidate.request.sourcePath = object.value(QStringLiteral("sourcePath")).toString();
    candidate.request.outputPath = object.value(QStringLiteral("outputPath")).toString();
    candidate.request.targetFormat = object.value(QStringLiteral("targetFormat")).toString();
    candidate.request.allowOverwrite = object.value(QStringLiteral("allowOverwrite")).toBool();
    candidate.message = object.value(QStringLiteral("message")).toString();
    if (candidate.request.sourcePath.isEmpty() || candidate.request.outputPath.isEmpty() || candidate.request.targetFormat.isEmpty()) return false;
    *item = candidate;
    return true;
}
}

ConversionQueue::ConversionQueue(QString statePath) : m_statePath(std::move(statePath)) {}
QString ConversionQueue::recordsPath() const { return m_statePath + QStringLiteral(".items"); }

bool ConversionQueue::enqueue(const ConversionRequest& request, QString* error)
{
    if (request.sourcePath.isEmpty() || request.outputPath.isEmpty() || request.targetFormat.isEmpty()
        || request.sourcePath.size() > MaxPathChars || request.outputPath.size() > MaxPathChars || request.targetFormat.size() > 32) {
        if (error) *error = QStringLiteral("A queue item has missing or oversized fields.");
        return false;
    }
    if (!QDir().mkpath(recordsPath())) { if (error) *error = QStringLiteral("The queue record directory could not be created."); return false; }
    QueueItem candidate;
    candidate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    candidate.request = request;
    return writeRecord(QDir(recordsPath()).filePath(candidate.id + QStringLiteral(".json")), candidate, error);
}

QVector<QueueItem> ConversionQueue::page(int offset, int limit) const
{
    QVector<QueueItem> result;
    if (offset < 0 || limit <= 0) return result;
    QDirIterator it(recordsPath(), { QStringLiteral("*.json") }, QDir::Files);
    int seen = 0;
    while (it.hasNext() && result.size() < qMin(limit, 500)) { QueueItem item; if (readRecord(it.next(), &item, nullptr) && seen++ >= offset) result.append(item); }
    return result;
}

int ConversionQueue::count() const
{
    int total = 0;
    QDirIterator it(recordsPath(), { QStringLiteral("*.json") }, QDir::Files);
    while (it.hasNext()) { QueueItem item; if (readRecord(it.next(), &item, nullptr)) ++total; }
    return total;
}

bool ConversionQueue::cancel(const QString& id)
{
    if (QUuid(id).isNull()) return false;
    const QString path = QDir(recordsPath()).filePath(id + QStringLiteral(".json"));
    QueueItem candidate;
    if (!readRecord(path, &candidate, nullptr) || (candidate.state != QueueItemState::Pending && candidate.state != QueueItemState::Running)) return false;
    candidate.state = QueueItemState::Cancelled;
    candidate.message = QStringLiteral("Cancelled by the user.");
    return writeRecord(path, candidate, nullptr);
}

bool ConversionQueue::load(QString* error)
{
    QDirIterator it(recordsPath(), { QStringLiteral("*.json") }, QDir::Files);
    while (it.hasNext()) { const QString path = it.next(); QueueItem item; if (!readRecord(path, &item, error)) return false;
        if (item.state == QueueItemState::Running) { item.state = QueueItemState::Pending; item.message = QStringLiteral("Recovered after restart."); if (!writeRecord(path, item, error)) return false; } }
    return true;
}

bool ConversionQueue::save(QString*) const { return true; }
