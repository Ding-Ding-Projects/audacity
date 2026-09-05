/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace au::toolkit {
//! One local harness launch profile: an allowlisted executable, a fixed
//! argument schema, a working directory and environment variable keys.
//! There is deliberately no field that can hold a shell command line, a
//! script body or command concatenation: a profile names one executable
//! and a list of literal arguments, nothing that a shell would interpret.
struct HarnessProfile {
    QString name;
    QString executablePath;
    QStringList arguments;
    QString workingDirectory;
    QStringList environmentKeys;
};

//! Characters and sequences that indicate the caller is trying to smuggle a
//! shell command through an argument or the executable path, rather than
//! naming a real binary with literal arguments.
bool argumentLooksLikeShell(const QString& value);

//! Returns an empty string when the profile is safe to launch, or a human
//! readable reason naming the exact rejected field otherwise.
QString validateHarnessProfile(const HarnessProfile& profile);

QVariantMap harnessProfileToVariantMap(const HarnessProfile& profile);
HarnessProfile harnessProfileFromVariantMap(const QVariantMap& map);
}
