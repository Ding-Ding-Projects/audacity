/*
* Audacity: A Digital Audio Editor
*/

#include "settingsindex.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

using namespace au::companion;

namespace {
const QString INDEX_RESOURCE = QStringLiteral(":/palette/settingsindex.json");
}

SettingsIndex SettingsIndex::fromJson(const QByteArray& json, QString* error)
{
    SettingsIndex index;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return index;
    }
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("the settings index must be a JSON object");
        }
        return index;
    }

    const QJsonObject root = document.object();

    const auto readArray = [&root](const QString& name) {
        QVariantList out;
        const QJsonArray array = root.value(name).toArray();
        for (const QJsonValue& value : array) {
            out.append(value.toObject().toVariantMap());
        }
        return out;
    };

    index.m_pages = readArray(QStringLiteral("pages"));
    index.m_settings = readArray(QStringLiteral("settings"));
    index.m_appearance = readArray(QStringLiteral("appearance"));

    return index;
}

const SettingsIndex& SettingsIndex::instance()
{
    static SettingsIndex index = []() {
        QFile file(INDEX_RESOURCE);
        if (!file.open(QIODevice::ReadOnly)) {
            return SettingsIndex();
        }
        const QByteArray data = file.readAll();
        file.close();
        return SettingsIndex::fromJson(data);
    }();
    return index;
}

QStringList SettingsIndex::targets() const
{
    QSet<QString> seen;
    QStringList out;
    const auto collect = [&seen, &out](const QVariantList& rows) {
        for (const QVariant& value : rows) {
            const QString target = value.toMap().value(QStringLiteral("target")).toString();
            if (target.isEmpty() || seen.contains(target)) {
                continue;
            }
            seen.insert(target);
            out.append(target);
        }
    };
    collect(m_settings);
    collect(m_appearance);
    return out;
}
