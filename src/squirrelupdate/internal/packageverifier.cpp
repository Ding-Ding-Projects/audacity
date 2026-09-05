/*
* Audacity: A Digital Audio Editor
*/
#include "packageverifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

namespace au::squirrelupdate {
namespace {
QString hashOfFile(const QString& filePath, QCryptographicHash::Algorithm algorithm)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(algorithm);
    if (!hash.addData(&file)) {
        return QString();
    }

    return QString::fromLatin1(hash.result().toHex()).toUpper();
}
}

QString PackageVerifier::sha1OfFile(const QString& filePath)
{
    return hashOfFile(filePath, QCryptographicHash::Sha1);
}

QString PackageVerifier::sha256OfFile(const QString& filePath)
{
    return hashOfFile(filePath, QCryptographicHash::Sha256);
}

bool PackageVerifier::verify(const QString& filePath, const ReleaseEntry& entry, QString* error)
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        if (error) {
            *error = QString("the downloaded package is missing: %1").arg(filePath);
        }
        return false;
    }

    if (entry.size > 0 && info.size() != entry.size) {
        if (error) {
            *error = QString("size mismatch: the feed lists %1 bytes, the file holds %2")
                     .arg(entry.size).arg(info.size());
        }
        return false;
    }

    const QString actual = sha1OfFile(filePath);
    if (actual.isEmpty()) {
        if (error) {
            *error = QString("the downloaded package could not be read: %1").arg(filePath);
        }
        return false;
    }

    if (actual.compare(entry.sha1, Qt::CaseInsensitive) != 0) {
        if (error) {
            *error = QString("SHA1 mismatch: the feed lists %1, the file hashes to %2")
                     .arg(entry.sha1, actual);
        }
        return false;
    }

    return true;
}
}
