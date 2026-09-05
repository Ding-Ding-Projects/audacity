/*
* Audacity: A Digital Audio Editor
*/

#include "harnessprofile.h"

#include <QFileInfo>

using namespace au::toolkit;

bool au::toolkit::argumentLooksLikeShell(const QString& value)
{
    static const QStringList shellMarkers = {
        QStringLiteral("|"), QStringLiteral("&&"), QStringLiteral("||"),
        QStringLiteral(";"), QStringLiteral("`"), QStringLiteral("$("),
        QStringLiteral(">"), QStringLiteral("<"), QStringLiteral("\n"),
        QStringLiteral("&"), QStringLiteral("$"), QStringLiteral("~")
    };

    for (const QString& marker : shellMarkers) {
        if (value.contains(marker)) {
            return true;
        }
    }
    return false;
}

QString au::toolkit::validateHarnessProfile(const HarnessProfile& profile)
{
    if (profile.name.trimmed().isEmpty()) {
        return QStringLiteral("The profile needs a name.");
    }

    if (profile.executablePath.trimmed().isEmpty()) {
        return QStringLiteral("The profile needs an executable, chosen through the file picker.");
    }

    if (argumentLooksLikeShell(profile.executablePath)) {
        return QStringLiteral("The executable path looks like a shell command rather than a single program; "
                              "pick the real executable file instead.");
    }

    for (const QString& arg : profile.arguments) {
        if (argumentLooksLikeShell(arg)) {
            return QStringLiteral("One argument looks like a shell command rather than a literal value: ") + arg;
        }
    }

    if (!profile.workingDirectory.isEmpty() && argumentLooksLikeShell(profile.workingDirectory)) {
        return QStringLiteral("The working directory looks like a shell command rather than a real path.");
    }

    return QString();
}

QVariantMap au::toolkit::harnessProfileToVariantMap(const HarnessProfile& profile)
{
    QVariantMap map;
    map[QStringLiteral("name")] = profile.name;
    map[QStringLiteral("executablePath")] = profile.executablePath;
    map[QStringLiteral("arguments")] = QVariant(profile.arguments);
    map[QStringLiteral("workingDirectory")] = profile.workingDirectory;
    map[QStringLiteral("environmentKeys")] = QVariant(profile.environmentKeys);
    return map;
}

HarnessProfile au::toolkit::harnessProfileFromVariantMap(const QVariantMap& map)
{
    HarnessProfile profile;
    profile.name = map.value(QStringLiteral("name")).toString();
    profile.executablePath = map.value(QStringLiteral("executablePath")).toString();
    profile.arguments = map.value(QStringLiteral("arguments")).toStringList();
    profile.workingDirectory = map.value(QStringLiteral("workingDirectory")).toString();
    profile.environmentKeys = map.value(QStringLiteral("environmentKeys")).toStringList();
    return profile;
}
