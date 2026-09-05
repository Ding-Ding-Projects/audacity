/*
* Audacity: A Digital Audio Editor
*/
#include "releasesparser.h"

#include <QRegularExpression>

namespace au::squirrelupdate {
namespace {
const QRegularExpression& sha1Pattern()
{
    static const QRegularExpression re("^[0-9a-fA-F]{40}$");
    return re;
}

const QRegularExpression& whitespacePattern()
{
    static const QRegularExpression re("[ \\t]+");
    return re;
}

const QRegularExpression& numericStartPattern()
{
    static const QRegularExpression re("^[0-9]+$");
    return re;
}

//! Splits "4.0.0-m3001" into the numeric part and the pre-release label.
void splitVersion(const QString& version, QStringList& numbers, QString& preRelease)
{
    const int dash = version.indexOf('-');
    const QString numeric = dash < 0 ? version : version.left(dash);
    preRelease = dash < 0 ? QString() : version.mid(dash + 1);
    numbers = numeric.split('.', Qt::SkipEmptyParts);
}

int comparePreRelease(const QString& a, const QString& b)
{
    // No label outranks any label: 4.0.0 is newer than 4.0.0-m3001.
    if (a.isEmpty() && b.isEmpty()) {
        return 0;
    }
    if (a.isEmpty()) {
        return 1;
    }
    if (b.isEmpty()) {
        return -1;
    }

    const QStringList aParts = a.split('.', Qt::SkipEmptyParts);
    const QStringList bParts = b.split('.', Qt::SkipEmptyParts);
    const int count = std::max(aParts.size(), bParts.size());

    for (int i = 0; i < count; ++i) {
        if (i >= aParts.size()) {
            return -1;
        }
        if (i >= bParts.size()) {
            return 1;
        }

        const QString& ap = aParts.at(i);
        const QString& bp = bParts.at(i);
        const bool aNum = numericStartPattern().match(ap).hasMatch();
        const bool bNum = numericStartPattern().match(bp).hasMatch();

        if (aNum && bNum) {
            const qlonglong an = ap.toLongLong();
            const qlonglong bn = bp.toLongLong();
            if (an != bn) {
                return an < bn ? -1 : 1;
            }
            continue;
        }

        // A numeric identifier ranks below an alphanumeric one.
        if (aNum != bNum) {
            return aNum ? -1 : 1;
        }

        const int cmp = QString::compare(ap, bp);
        if (cmp != 0) {
            return cmp < 0 ? -1 : 1;
        }
    }

    return 0;
}
}

ReleaseEntry ReleasesParser::parseLine(const QString& line, QString* error)
{
    ReleaseEntry entry;

    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#')) {
        return entry;
    }

    const QStringList fields = trimmed.split(whitespacePattern(), Qt::SkipEmptyParts);
    if (fields.size() != 3) {
        if (error) {
            *error = QString("expected three fields, found %1").arg(fields.size());
        }
        return entry;
    }

    if (!sha1Pattern().match(fields.at(0)).hasMatch()) {
        if (error) {
            *error = "the first field is not a 40 character SHA1";
        }
        return entry;
    }

    const QString fileName = fields.at(1);
    if (!fileName.endsWith(".nupkg", Qt::CaseInsensitive)) {
        if (error) {
            *error = "the file name does not end in .nupkg";
        }
        return entry;
    }

    bool sizeOk = false;
    const qlonglong size = fields.at(2).toLongLong(&sizeOk);
    if (!sizeOk || size <= 0) {
        if (error) {
            *error = "the size field is not a positive integer";
        }
        return entry;
    }

    const QString version = versionFromFileName(fileName);
    if (version.isEmpty()) {
        if (error) {
            *error = "no version could be read from the file name";
        }
        return entry;
    }

    entry.sha1 = fields.at(0).toUpper();
    entry.fileName = fileName;
    entry.size = size;
    entry.version = version;
    entry.isDelta = fileName.contains("-delta.nupkg", Qt::CaseInsensitive);

    return entry;
}

ReleaseEntryList ReleasesParser::parse(const QString& text, QStringList* errors)
{
    ReleaseEntryList result;

    const QStringList lines = text.split(QRegularExpression("\r\n|\n|\r"));
    for (int i = 0; i < lines.size(); ++i) {
        QString error;
        const ReleaseEntry entry = parseLine(lines.at(i), &error);
        if (entry.isValid()) {
            result.append(entry);
        } else if (!error.isEmpty() && errors) {
            errors->append(QString("line %1: %2").arg(i + 1).arg(error));
        }
    }

    return result;
}

QString ReleasesParser::versionFromFileName(const QString& fileName)
{
    QString name = fileName;

    // A feed entry may name a full URL. Only the last path segment matters.
    const int slash = std::max(name.lastIndexOf('/'), name.lastIndexOf('\\'));
    if (slash >= 0) {
        name = name.mid(slash + 1);
    }

    if (name.endsWith(".nupkg", Qt::CaseInsensitive)) {
        name.chop(QString(".nupkg").size());
    }

    for (const QString& suffix : { QStringLiteral("-full"), QStringLiteral("-delta") }) {
        if (name.endsWith(suffix, Qt::CaseInsensitive)) {
            name.chop(suffix.size());
            break;
        }
    }

    // The package id may itself contain hyphens, so the version starts at the
    // first segment that looks like a number.
    const QStringList parts = name.split('-');
    for (int i = 0; i < parts.size(); ++i) {
        const QString& part = parts.at(i);
        if (!part.isEmpty() && part.at(0).isDigit() && part.contains('.')) {
            return QStringList(parts.mid(i)).join('-');
        }
    }

    return QString();
}

int ReleasesParser::compareVersions(const QString& a, const QString& b)
{
    QStringList aNumbers;
    QStringList bNumbers;
    QString aPre;
    QString bPre;
    splitVersion(a, aNumbers, aPre);
    splitVersion(b, bNumbers, bPre);

    const int count = std::max(aNumbers.size(), bNumbers.size());
    for (int i = 0; i < count; ++i) {
        const qlonglong an = i < aNumbers.size() ? aNumbers.at(i).toLongLong() : 0;
        const qlonglong bn = i < bNumbers.size() ? bNumbers.at(i).toLongLong() : 0;
        if (an != bn) {
            return an < bn ? -1 : 1;
        }
    }

    return comparePreRelease(aPre, bPre);
}

ReleaseEntry ReleasesParser::newest(const ReleaseEntryList& entries)
{
    ReleaseEntry best;
    for (const ReleaseEntry& entry : entries) {
        if (!entry.isValid()) {
            continue;
        }
        if (!best.isValid()) {
            best = entry;
            continue;
        }

        const int cmp = compareVersions(entry.version, best.version);
        if (cmp > 0) {
            best = entry;
        } else if (cmp == 0 && best.isDelta && !entry.isDelta) {
            // A full package of the same version is preferred, because it can
            // always be applied while a delta needs the exact previous release.
            best = entry;
        }
    }

    return best;
}

ReleaseEntry ReleasesParser::newestAfter(const ReleaseEntryList& entries, const QString& installedVersion)
{
    ReleaseEntryList newer;
    for (const ReleaseEntry& entry : entries) {
        if (entry.isValid() && compareVersions(entry.version, installedVersion) > 0) {
            newer.append(entry);
        }
    }

    return newest(newer);
}
}
