#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QTimeZone>

namespace au::appshell {
// Values are accepted together from the exact generated manifest bytes.
class BuildProvenance
{
public:
    static BuildProvenance fromManifest(const QByteArray& json, const QByteArray& expectedSha256);
    bool isValid() const { return !m_version.isEmpty() && m_builtAt.isValid(); }
    QString version() const { return isValid() ? m_version : QString(); }
    QString updatedAtUtc() const;
    QString updatedAtLocal(const QTimeZone& zone = QTimeZone::systemTimeZone()) const;

private:
    QString m_version;
    QDateTime m_builtAt;
};
}
