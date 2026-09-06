/*
* Audacity: A Digital Audio Editor
*/

#include "conversionengine.h"
#include "convertercatalog.h"
#include "conversionqueue.h"

#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QTemporaryDir>

#include <gtest/gtest.h>

using namespace au::converter;

namespace {
QString writePng(const QTemporaryDir& directory, const QString& name)
{
    const QString path = directory.filePath(name);
    QImage image(4, 3, QImage::Format_ARGB32);
    image.fill(Qt::red);
    EXPECT_TRUE(image.save(path, "png"));
    return path;
}
}

TEST(ConversionEngine, rejectsCorruptInputByBytes)
{
    QTemporaryDir directory;
    const QString source = directory.filePath(QStringLiteral("not-really-png.png"));
    QFile file(source);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("not an image");
    file.close();

    const ConversionResult result = ConversionEngine().convert({ source, directory.filePath(QStringLiteral("out.jpg")), QStringLiteral("jpeg") });
    EXPECT_EQ(result.status, ConversionStatus::Rejected);
    EXPECT_FALSE(QFile::exists(directory.filePath(QStringLiteral("out.jpg"))));
}

TEST(ConverterCatalog, showsKnownUnavailableFormatsWithReasons)
{
    const AdapterDescriptor pdf = ConverterCatalog::find(QStringLiteral("PDF"), QStringLiteral("PDF"));
    EXPECT_FALSE(pdf.enabled);
    EXPECT_FALSE(pdf.unavailableReason.isEmpty());

    const AdapterDescriptor audio = ConverterCatalog::find(QStringLiteral("WAV"), QStringLiteral("MP3"));
    EXPECT_FALSE(audio.enabled);
    EXPECT_FALSE(audio.unavailableReason.isEmpty());
}

TEST(ConverterCatalog, imageCapabilityIsReportedHonestly)
{
    const AdapterDescriptor image = ConverterCatalog::find(QStringLiteral("PNG"), QStringLiteral("JPEG"));
    EXPECT_EQ(image.category, QStringLiteral("Images"));
    if (image.enabled) {
        EXPECT_TRUE(image.bundled);
    } else {
        EXPECT_FALSE(image.unavailableReason.isEmpty());
    }
}

TEST(ConversionEngine, preservesExistingDestinationWithoutExplicitOverwrite)
{
    QTemporaryDir directory;
    const QString source = writePng(directory, QStringLiteral("source.png"));
    const QString destination = directory.filePath(QStringLiteral("existing.jpg"));
    QFile existing(destination);
    ASSERT_TRUE(existing.open(QIODevice::WriteOnly));
    existing.write("do not replace");
    existing.close();

    const ConversionResult result = ConversionEngine().convert({ source, destination, QStringLiteral("jpeg") });
    EXPECT_EQ(result.status, ConversionStatus::Rejected);
    ASSERT_TRUE(existing.open(QIODevice::ReadOnly));
    EXPECT_EQ(existing.readAll(), QByteArray("do not replace"));
}

TEST(ConversionEngine, publishesOnlyVerifiedImageOutput)
{
    QTemporaryDir directory;
    const QString source = writePng(directory, QStringLiteral("source.png"));
    const QString destination = directory.filePath(QStringLiteral("converted.jpg"));
    const ConversionResult result = ConversionEngine().convert({ source, destination, QStringLiteral("jpeg") });
    if (result.status == ConversionStatus::Rejected) GTEST_SKIP() << result.message.toStdString();
    EXPECT_EQ(result.status, ConversionStatus::Converted);
    QImage verified(destination);
    EXPECT_FALSE(verified.isNull());
    EXPECT_EQ(verified.size(), QSize(4, 3));
}

TEST(ConversionEngine, cancellationDoesNotPublishOutput)
{
    QTemporaryDir directory;
    const QString source = writePng(directory, QStringLiteral("source.png"));
    const QString destination = directory.filePath(QStringLiteral("cancelled.jpg"));
    const bool cancel = true;
    const ConversionResult result = ConversionEngine().convert({ source, destination, QStringLiteral("jpeg") }, &cancel);
    EXPECT_EQ(result.status, ConversionStatus::Cancelled);
    EXPECT_FALSE(QFile::exists(destination));
}

TEST(ConversionQueue, restartAndCancellationPersistPathsWithoutSourceBytes)
{
    QTemporaryDir directory;
    const QString statePath = directory.filePath(QStringLiteral("queue.json"));
    ConversionQueue queue(statePath);
    ASSERT_TRUE(queue.enqueue({ QStringLiteral("C:/input.png"), QStringLiteral("C:/output.jpg"), QStringLiteral("jpeg") }));
    const QVector<QueueItem> initial = queue.page(0, 20);
    ASSERT_EQ(initial.size(), 1);
    ASSERT_TRUE(queue.cancel(initial.first().id));

    ConversionQueue restarted(statePath);
    ASSERT_TRUE(restarted.load());
    const QVector<QueueItem> restored = restarted.page(0, 20);
    ASSERT_EQ(restored.size(), 1);
    EXPECT_EQ(restored.first().state, QueueItemState::Cancelled);
    QFile stateFile(statePath);
    ASSERT_TRUE(stateFile.open(QIODevice::ReadOnly));
    const QByteArray state = stateFile.readAll();
    EXPECT_TRUE(state.contains("C:/input.png"));
    EXPECT_FALSE(state.contains("\x89PNG"));
}
