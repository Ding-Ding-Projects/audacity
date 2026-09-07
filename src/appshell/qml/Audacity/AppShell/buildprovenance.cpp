#include "buildprovenance.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStringList>

namespace au::appshell {
BuildProvenance BuildProvenance::fromManifest(const QByteArray& json, const QByteArray& expectedSha256)
{
    if (json.isEmpty() || json.size() > 16384 || expectedSha256.size() != 64
        || QCryptographicHash::hash(json, QCryptographicHash::Sha256).toHex() != expectedSha256.toLower()) {
        return {};
    }
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }
    const auto object = document.object();
    const QStringList fields { "schemaVersion", "version", "versionLabel", "buildNumber", "buildId", "sourceRevision",
                              "sourceTree", "buildStartedAtUtc", "timestampKind" };
    if (object.size() != fields.size() || object.value("schemaVersion").toInt(-1) != 1) {
        return {};
    }
    for (const auto& field : fields) {
        if (field != "schemaVersion" && !object.value(field).isString()) {
            return {};
        }
    }
    const auto version = object.value("version").toString();
    const auto label = object.value("versionLabel").toString();
    const auto build = object.value("buildNumber").toString();
    static const QRegularExpression versionPattern(QStringLiteral("\\A(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\z"));
    static const QRegularExpression labelPattern(QStringLiteral("\\A[0-9A-Za-z][0-9A-Za-z.-]*\\z"));
    static const QRegularExpression numberPattern(QStringLiteral("\\A[0-9]+\\z"));
    static const QRegularExpression revisionPattern(QStringLiteral("\\A[0-9a-f]{40}\\z"));
    static const QRegularExpression buildIdPattern(QStringLiteral("\\A[0-9a-f]{32}\\z"));
    if (version.size() > 32 || !versionPattern.match(version).hasMatch()
        || label.size() > 32 || (!label.isEmpty() && !labelPattern.match(label).hasMatch())
        || build.size() > 20 || (!build.isEmpty() && !numberPattern.match(build).hasMatch())
        || !buildIdPattern.match(object.value("buildId").toString()).hasMatch()
        || object.value("timestampKind").toString() != QStringLiteral("build-start")) {
        return {};
    }
    for (const auto& field : { "sourceRevision", "sourceTree" }) {
        const auto value = object.value(field).toString();
        if (!revisionPattern.match(value).hasMatch() || value == QString(40, QLatin1Char('0'))) {
            return {};
        }
    }
    const auto timestamp = object.value("buildStartedAtUtc").toString();
    // An explicit offset is required. Never infer the local timezone from a
    // timezone-less string, or append a UTC label without converting it.
    static const QRegularExpression timestampPattern(QStringLiteral(
        "\\A[0-9]{4}-[0-9]{2}-[0-9]{2}T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](Z|[+-]([01][0-9]|2[0-3]):[0-5][0-9])\\z"));
    if (!timestampPattern.match(timestamp).hasMatch()) {
        return {};
    }
    const auto dateTime = QDateTime::fromString(timestamp, Qt::ISODate);
    if (!dateTime.isValid()) {
        return {};
    }
    BuildProvenance result;
    result.m_version = version;
    if (!label.isEmpty()) {
        result.m_version += QLatin1Char('-') + label;
    }
    if (!build.isEmpty()) {
        result.m_version += QLatin1Char('.') + build;
    }
    result.m_builtAt = dateTime.toUTC();
    return result;
}

QString BuildProvenance::updatedAtUtc() const
{
    return isValid() ? m_builtAt.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss 'UTC'")) : QString();
}

QString BuildProvenance::updatedAtLocal(const QTimeZone& zone) const
{
    if (!isValid() || !zone.isValid()) {
        return {};
    }
    const auto local = m_builtAt.toTimeZone(zone);
    return local.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss 'UTC'ttt"))
           + QStringLiteral(" (") + QString::fromUtf8(zone.id()) + QLatin1Char(')');
}
}
