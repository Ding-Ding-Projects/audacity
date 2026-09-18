/*
 * Audacity: A Digital Audio Editor
 */
#include "personalvocabulary.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

#include <algorithm>

#include "framework/global/translation.h"

namespace au::experience {
class PersonalVocabulary::Matcher
{
public:
    struct Entry {
        QString from;
        QString to;
        bool startsWithAsciiWordCharacter = false;
        bool endsWithAsciiWordCharacter = false;
    };

    struct Node {
        QHash<ushort, int> children;
        int entryIndex = -1;
    };

    QVector<Entry> entries;
    QVector<Node> nodes { Node {} };
};

namespace {
bool isAsciiWordCharacter(const QChar ch)
{
    return (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z')) || (ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
           || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) || ch == QLatin1Char('_');
}

bool hasWholeWordBoundaries(const QString& text, const int start, const int end, const PersonalVocabulary::Matcher::Entry& entry)
{
    return (!entry.startsWithAsciiWordCharacter || start == 0 || !isAsciiWordCharacter(text.at(start - 1)))
           && (!entry.endsWithAsciiWordCharacter || end == text.size() || !isAsciiWordCharacter(text.at(end)));
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
        if (error.error != QJsonParseError::NoError || !doc.isArray() || doc.array().size() != 1 || !doc.array().at(0).isString()) {
            return false;
        }
        *decoded = doc.array().at(0).toString();
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

PersonalVocabulary::MatcherPtr PersonalVocabulary::compile(const Table& entries)
{
    if (entries.isEmpty()) {
        return {};
    }

    auto matcher = std::make_shared<Matcher>();
    matcher->entries.reserve(entries.size());
    for (const auto& tableEntry : entries) {
        if (tableEntry.first.isEmpty()) {
            continue;
        }

        Matcher::Entry entry;
        entry.from = tableEntry.first;
        entry.to = tableEntry.second;
        entry.startsWithAsciiWordCharacter = isAsciiWordCharacter(entry.from.at(0));
        entry.endsWithAsciiWordCharacter = isAsciiWordCharacter(entry.from.at(entry.from.size() - 1));
        const int entryIndex = matcher->entries.size();
        matcher->entries.append(std::move(entry));

        int nodeIndex = 0;
        for (const QChar character : tableEntry.first) {
            const ushort key = character.unicode();
            const auto child = matcher->nodes.at(nodeIndex).children.constFind(key);
            if (child == matcher->nodes.at(nodeIndex).children.constEnd()) {
                const int nextNodeIndex = matcher->nodes.size();
                matcher->nodes.append(Matcher::Node {});
                matcher->nodes[nodeIndex].children.insert(key, nextNodeIndex);
                nodeIndex = nextNodeIndex;
            } else {
                nodeIndex = child.value();
            }
        }
        //! Keep the first duplicate table entry. Parsed documents cannot have
        //! duplicates, and this makes the public Table API deterministic.
        if (matcher->nodes[nodeIndex].entryIndex < 0) {
            matcher->nodes[nodeIndex].entryIndex = entryIndex;
        }
    }
    return matcher->entries.isEmpty() ? MatcherPtr {} : matcher;
}

QString PersonalVocabulary::apply(const QString& text, const Table& entries)
{
    return apply(text, compile(entries));
}

QString PersonalVocabulary::apply(const QString& text, const MatcherPtr& matcher)
{
    if (text.isEmpty() || !matcher) {
        return text;
    }

    QString result;
    result.reserve(text.size());
    for (int start = 0; start < text.size();) {
        int nodeIndex = 0;
        int bestEntryIndex = -1;
        int bestEnd = start;
        for (int end = start; end < text.size(); ++end) {
            const auto child = matcher->nodes.at(nodeIndex).children.constFind(text.at(end).unicode());
            if (child == matcher->nodes.at(nodeIndex).children.constEnd()) {
                break;
            }
            nodeIndex = child.value();
            const int entryIndex = matcher->nodes.at(nodeIndex).entryIndex;
            if (entryIndex >= 0 && hasWholeWordBoundaries(text, start, end + 1, matcher->entries.at(entryIndex))) {
                bestEntryIndex = entryIndex;
                bestEnd = end + 1;
            }
        }

        if (bestEntryIndex >= 0) {
            result.append(matcher->entries.at(bestEntryIndex).to);
            start = bestEnd;
        } else {
            result.append(text.at(start));
            ++start;
        }
    }
    return result;
}
}
