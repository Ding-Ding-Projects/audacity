/*
 * Audacity: A Digital Audio Editor
 */
#include "personalvocabulary.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

#include "framework/global/translation.h"

namespace au::experience {
namespace {
bool startsWithWordCharacter(const QString& text)
{
    return !text.isEmpty() && (text.at(0).isLetterOrNumber() || text.at(0) == QLatin1Char('_'));
}

bool endsWithWordCharacter(const QString& text)
{
    return !text.isEmpty() && (text.at(text.size() - 1).isLetterOrNumber() || text.at(text.size() - 1) == QLatin1Char('_'));
}

//! A word boundary is only meaningful where the term is bounded by letters or
//! digits. Chinese text has no spaces, so a term made of Chinese characters is
//! matched literally instead.
QRegularExpression patternFor(const QString& from)
{
    QString pattern = QRegularExpression::escape(from);
    if (startsWithWordCharacter(from)) {
        pattern.prepend(QStringLiteral("(?<![0-9A-Za-z_])"));
    }
    if (endsWithWordCharacter(from)) {
        pattern.append(QStringLiteral("(?![0-9A-Za-z_])"));
    }
    return QRegularExpression(pattern);
}

bool hasControl(const QString& text)
{
    for (const QChar ch : text) {
        const ushort value = ch.unicode();
        if ((value <= 0x08) || value == 0x0b || value == 0x0c || (value >= 0x0e && value <= 0x1f) || value == 0x7f) {
            return true;
        }
    }
    return false;
}

constexpr int MAX_JSON_DEPTH = 8;

void skipWhitespace(const QByteArray& data, int& index)
{
    while (index < data.size() && QByteArray(" \t\r\n").contains(data.at(index))) {
        ++index;
    }
}

bool scanJsonString(const QByteArray& data, int& index, QString* decoded = nullptr)
{
    if (index >= data.size() || data.at(index) != '"') {
        return false;
    }

    const int start = index++;
    bool escaped = false;
    while (index < data.size()) {
        const char ch = data.at(index++);
        if (!escaped && ch == '"') {
            break;
        }
        if (!escaped && ch == '\\') {
            escaped = true;
        } else {
            escaped = false;
        }
    }
    if (index > data.size() || data.at(index - 1) != '"') {
        return false;
    }

    if (decoded) {
        QJsonParseError error;
        const QByteArray quoted = data.mid(start, index - start);
        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray("[") + quoted + QByteArray("]"), &error);
        if (error.error != QJsonParseError::NoError || !doc.isArray() || doc.array().size() != 1 || !doc.array().front().isString()) {
            return false;
        }
        *decoded = doc.array().front().toString();
    }
    return true;
}

bool scanJsonValue(const QByteArray& data, int& index, int depth);

bool scanJsonObject(const QByteArray& data, int& index, int depth)
{
    if (depth > MAX_JSON_DEPTH || index >= data.size() || data.at(index++) != '{') {
        return false;
    }

    skipWhitespace(data, index);
    QSet<QString> keys;
    if (index < data.size() && data.at(index) == '}') {
        ++index;
        return true;
    }
    while (index < data.size()) {
        QString key;
        if (!scanJsonString(data, index, &key) || keys.contains(key)) {
            return false;
        }
        keys.insert(key);
        skipWhitespace(data, index);
        if (index >= data.size() || data.at(index++) != ':') {
            return false;
        }
        if (!scanJsonValue(data, index, depth + 1)) {
            return false;
        }
        skipWhitespace(data, index);
        if (index < data.size() && data.at(index) == '}') {
            ++index;
            return true;
        }
        if (index >= data.size() || data.at(index++) != ',') {
            return false;
        }
        skipWhitespace(data, index);
    }
    return false;
}

bool scanJsonArray(const QByteArray& data, int& index, int depth)
{
    if (depth > MAX_JSON_DEPTH || index >= data.size() || data.at(index++) != '[') {
        return false;
    }
    skipWhitespace(data, index);
    if (index < data.size() && data.at(index) == ']') {
        ++index;
        return true;
    }
    while (index < data.size()) {
        if (!scanJsonValue(data, index, depth + 1)) {
            return false;
        }
        skipWhitespace(data, index);
        if (index < data.size() && data.at(index) == ']') {
            ++index;
            return true;
        }
        if (index >= data.size() || data.at(index++) != ',') {
            return false;
        }
        skipWhitespace(data, index);
    }
    return false;
}

bool scanJsonValue(const QByteArray& data, int& index, int depth)
{
    skipWhitespace(data, index);
    if (index >= data.size() || depth > MAX_JSON_DEPTH) {
        return false;
    }
    if (data.at(index) == '{') {
        return scanJsonObject(data, index, depth);
    }
    if (data.at(index) == '[') {
        return scanJsonArray(data, index, depth);
    }
    if (data.at(index) == '"') {
        return scanJsonString(data, index);
    }

    const int start = index;
    while (index < data.size() && !QByteArray(" \t\r\n,]}").contains(data.at(index))) {
        ++index;
    }
    return index > start;
}

bool hasValidJsonStructure(const QByteArray& data)
{
    int index = 0;
    if (!scanJsonValue(data, index, 1)) {
        return false;
    }
    skipWhitespace(data, index);
    return index == data.size();
}

bool validEntryText(const QString& from, const QString& to)
{
    return !from.isEmpty() && from.size() <= PersonalVocabulary::MAX_SOURCE_LENGTH
           && to.size() <= PersonalVocabulary::MAX_REPLACEMENT_LENGTH && !hasControl(from) && !hasControl(to)
           && from != QStringLiteral("__proto__") && from != QStringLiteral("prototype") && from != QStringLiteral("constructor");
}

PersonalVocabulary::ParseResult parseDocument(const QByteArray& data, bool allowLegacyCache)
{
    PersonalVocabulary::ParseResult result;

    if (data.isEmpty()) {
        result.error = muse::qtrc("experience", "The file is empty.");
        return result;
    }
    if (data.size() > PersonalVocabulary::MAX_BYTES) {
        result.error = muse::qtrc("experience", "The file is larger than 256 KB.");
        return result;
    }
    const QString utf8 = QString::fromUtf8(data);
    if (utf8.toUtf8() != data) {
        result.error = muse::qtrc("experience", "The file has invalid UTF-8.");
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !hasValidJsonStructure(data)) {
        result.error = muse::qtrc("experience", "The file is not valid JSON.");
        return result;
    }
    if (!doc.isObject()) {
        result.error = muse::qtrc("experience", "The top level of the file must be an object.");
        return result;
    }

    const QJsonObject root = doc.object();
    const QJsonValue schemaVersion = root.value(QStringLiteral("schemaVersion"));
    const QJsonValue entriesValue = root.value(QStringLiteral("entries"));
    if (root.size() == 2 && schemaVersion.isDouble() && schemaVersion.toDouble() == 1.0 && entriesValue.isObject()) {
        const QJsonObject entries = entriesValue.toObject();
        if (entries.size() > PersonalVocabulary::MAX_ENTRIES) {
            result.error = muse::qtrc("experience", "The file has too many entries.");
            return result;
        }
        for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
            const QString from = it.key();
            if (!it.value().isString() || !validEntryText(from, it.value().toString())) {
                result.error = muse::qtrc("experience", "The file has invalid entry text.");
                return result;
            }
            result.entries.append({ from, it.value().toString() });
        }
    } else if (allowLegacyCache && root.size() == 2 && root.value(QStringLiteral("version")).isDouble()
               && root.value(QStringLiteral("version")).toDouble() == 1.0 && entriesValue.isArray()) {
        const QJsonArray legacy = entriesValue.toArray();
        if (legacy.size() > PersonalVocabulary::MAX_ENTRIES) {
            result.error = muse::qtrc("experience", "The legacy cache has too many entries.");
            return result;
        }
        QSet<QString> sources;
        for (const QJsonValue& value : legacy) {
            const QJsonObject entry = value.toObject();
            if (!value.isObject() || entry.size() != 2 || !entry.value(QStringLiteral("from")).isString()
                || !entry.value(QStringLiteral("to")).isString()) {
                result.error = muse::qtrc("experience", "The legacy cache has invalid entries.");
                return result;
            }
            const QString from = entry.value(QStringLiteral("from")).toString();
            const QString to = entry.value(QStringLiteral("to")).toString();
            if (!validEntryText(from, to) || sources.contains(from)) {
                result.error = muse::qtrc("experience", "The legacy cache has invalid entry text.");
                return result;
            }
            sources.insert(from);
            result.entries.append({ from, to });
        }
        result.migratedLegacy = true;
    } else {
        result.error = muse::qtrc("experience", "Only schema version 1 files are supported.");
        return result;
    }

    std::stable_sort(result.entries.begin(), result.entries.end(),
                     [](const std::pair<QString, QString>& a, const std::pair<QString, QString>& b) {
        return a.first.size() > b.first.size();
    });
    result.ok = true;
    return result;
}
}

PersonalVocabulary::ParseResult PersonalVocabulary::parse(const QByteArray& data)
{
    return parseDocument(data, false);
}

PersonalVocabulary::ParseResult PersonalVocabulary::parseStoredCache(const QByteArray& data)
{
    return parseDocument(data, true);
}

QByteArray PersonalVocabulary::serialize(const Table& entries)
{
    QJsonObject object;
    for (const auto& entry : entries) {
        object[entry.first] = entry.second;
    }

    QJsonObject root;
    root[QStringLiteral("schemaVersion")] = 1;
    root[QStringLiteral("entries")] = object;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QString PersonalVocabulary::apply(const QString& text, const Table& entries)
{
    if (text.isEmpty() || entries.isEmpty()) {
        return text;
    }

    QString result = text;
    for (const auto& entry : entries) {
        result.replace(patternFor(entry.first), entry.second);
    }
    return result;
}
}
