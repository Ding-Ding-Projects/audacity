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
}

PersonalVocabulary::ParseResult PersonalVocabulary::parse(const QByteArray& data)
{
    ParseResult result;

    if (data.isEmpty()) {
        result.error = muse::qtrc("experience", "The file is empty.");
        return result;
    }

    if (data.size() > MAX_BYTES) {
        result.error = muse::qtrc("experience", "The file is larger than 256 KB.");
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.error = muse::qtrc("experience", "The file is not valid JSON.");
        return result;
    }

    if (!doc.isObject()) {
        result.error = muse::qtrc("experience", "The top level of the file must be an object.");
        return result;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("version")).toInt(-1) != 1) {
        result.error = muse::qtrc("experience", "Only version 1 files are supported.");
        return result;
    }

    const QJsonValue entriesValue = root.value(QStringLiteral("entries"));
    if (!entriesValue.isArray()) {
        result.error = muse::qtrc("experience", "The file has no entries array.");
        return result;
    }

    const QJsonArray entries = entriesValue.toArray();
    if (entries.size() > MAX_ENTRIES) {
        result.error = muse::qtrc("experience", "The file has more than 2000 entries.");
        return result;
    }

    QSet<QString> seen;
    for (const QJsonValue& value : entries) {
        if (!value.isObject()) {
            result.error = muse::qtrc("experience", "Every entry must be an object.");
            return result;
        }

        const QJsonObject entry = value.toObject();
        const QJsonValue fromValue = entry.value(QStringLiteral("from"));
        const QJsonValue toValue = entry.value(QStringLiteral("to"));
        if (!fromValue.isString() || !toValue.isString()) {
            result.error = muse::qtrc("experience", "Every entry needs a text \"from\" and a text \"to\".");
            return result;
        }

        const QString from = fromValue.toString();
        const QString to = toValue.toString();
        if (from.isEmpty()) {
            result.error = muse::qtrc("experience", "An entry has an empty \"from\".");
            return result;
        }

        if (seen.contains(from)) {
            result.error = muse::qtrc("experience", "The same \"from\" appears more than once.");
            return result;
        }
        seen.insert(from);

        result.entries.append({ from, to });
    }

    if (result.entries.isEmpty()) {
        result.error = muse::qtrc("experience", "The file has no entries.");
        return result;
    }

    //! Longer terms first, so that a longer phrase is never cut in half by a
    //! shorter one it contains.
    std::stable_sort(result.entries.begin(), result.entries.end(),
                     [](const std::pair<QString, QString>& a, const std::pair<QString, QString>& b) {
        return a.first.size() > b.first.size();
    });

    result.ok = true;
    return result;
}

QByteArray PersonalVocabulary::serialize(const Table& entries)
{
    QJsonArray array;
    for (const auto& entry : entries) {
        QJsonObject object;
        object[QStringLiteral("from")] = entry.first;
        object[QStringLiteral("to")] = entry.second;
        array.append(object);
    }

    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("entries")] = array;
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
