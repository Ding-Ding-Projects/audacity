/*
* Audacity: A Digital Audio Editor
*/
#include "squirrelinstalllayout.h"

#include <QDir>
#include <QRegularExpression>
#include <QStringList>

namespace au::squirrelupdate {
namespace {
const QRegularExpression& appDirPattern()
{
    static const QRegularExpression re("^app-(.+)$", QRegularExpression::CaseInsensitiveOption);
    return re;
}

//! Returns the index of the nearest app-<version> segment, or -1.
int appSegmentIndex(const QStringList& segments)
{
    for (int i = segments.size() - 1; i >= 0; --i) {
        if (appDirPattern().match(segments.at(i)).hasMatch()) {
            return i;
        }
    }
    return -1;
}

QStringList segmentsOf(const QString& path)
{
    const QString clean = QDir::fromNativeSeparators(path);
    return clean.split('/', Qt::SkipEmptyParts);
}
}

QString SquirrelInstallLayout::versionFromPath(const QString& executableDir)
{
    const QStringList segments = segmentsOf(executableDir);
    const int index = appSegmentIndex(segments);
    if (index < 0) {
        return QString();
    }

    return appDirPattern().match(segments.at(index)).captured(1);
}

QString SquirrelInstallLayout::appDirFromPath(const QString& executableDir)
{
    const QString clean = QDir::fromNativeSeparators(executableDir);
    const QStringList segments = segmentsOf(clean);
    const int index = appSegmentIndex(segments);
    if (index < 0) {
        return QString();
    }

    const QString joined = QStringList(segments.mid(0, index + 1)).join('/');
    return clean.startsWith('/') ? '/' + joined : joined;
}

QString SquirrelInstallLayout::rootDirFromPath(const QString& executableDir)
{
    const QString appDir = appDirFromPath(executableDir);
    if (appDir.isEmpty()) {
        return QString();
    }

    const int slash = appDir.lastIndexOf('/');
    if (slash <= 0) {
        return QString();
    }

    return appDir.left(slash);
}
}
