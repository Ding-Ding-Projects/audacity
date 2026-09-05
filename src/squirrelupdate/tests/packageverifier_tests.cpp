/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>

#include "internal/packageverifier.h"

using namespace au::squirrelupdate;

namespace {
QString writeFile(const QString& dir, const QString& name, const QByteArray& content)
{
    const QString path = dir + "/" + name;
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(content);
    file.close();
    return path;
}
}

class PackageVerifierTests : public ::testing::Test
{
protected:
    QTemporaryDir dir;
};

TEST_F(PackageVerifierTests, Sha1AndSha256OfARealFile)
{
    const QByteArray content = "hello squirrel";
    const QString path = writeFile(dir.path(), "a.bin", content);

    const QString sha1 = PackageVerifier::sha1OfFile(path);
    const QString sha256 = PackageVerifier::sha256OfFile(path);

    EXPECT_EQ(sha1, QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex()).toUpper());
    EXPECT_EQ(sha256, QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex()).toUpper());
}

TEST_F(PackageVerifierTests, HashOfAMissingFileIsEmpty)
{
    EXPECT_TRUE(PackageVerifier::sha1OfFile(dir.path() + "/missing.bin").isEmpty());
}

TEST_F(PackageVerifierTests, VerifyAcceptsAMatchingFile)
{
    const QByteArray content = "package bytes";
    const QString path = writeFile(dir.path(), "p.nupkg", content);

    ReleaseEntry entry;
    entry.sha1 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex()).toUpper();
    entry.fileName = "p.nupkg";
    entry.size = content.size();
    entry.version = "4.0.0";

    QString error;
    EXPECT_TRUE(PackageVerifier::verify(path, entry, &error));
    EXPECT_TRUE(error.isEmpty());
}

TEST_F(PackageVerifierTests, VerifyRejectsAMissingFile)
{
    ReleaseEntry entry;
    entry.sha1 = QString(40, QChar('a'));
    entry.fileName = "missing.nupkg";
    entry.size = 5;
    entry.version = "4.0.0";

    QString error;
    EXPECT_FALSE(PackageVerifier::verify(dir.path() + "/missing.nupkg", entry, &error));
    EXPECT_FALSE(error.isEmpty());
}

TEST_F(PackageVerifierTests, VerifyRejectsASizeMismatch)
{
    const QByteArray content = "twelve bytes";
    const QString path = writeFile(dir.path(), "p.nupkg", content);

    ReleaseEntry entry;
    entry.sha1 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex()).toUpper();
    entry.fileName = "p.nupkg";
    entry.size = content.size() + 1;
    entry.version = "4.0.0";

    QString error;
    EXPECT_FALSE(PackageVerifier::verify(path, entry, &error));
    EXPECT_NE(error.indexOf("size mismatch"), -1);
}

TEST_F(PackageVerifierTests, VerifyRejectsAHashMismatch)
{
    const QByteArray content = "some bytes";
    const QString path = writeFile(dir.path(), "p.nupkg", content);

    ReleaseEntry entry;
    entry.sha1 = QString(40, QChar('0'));
    entry.fileName = "p.nupkg";
    entry.size = content.size();
    entry.version = "4.0.0";

    QString error;
    EXPECT_FALSE(PackageVerifier::verify(path, entry, &error));
    EXPECT_NE(error.indexOf("SHA1 mismatch"), -1);
}
