/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include "internal/ollamaclient.h"

using namespace au::toolkit;

namespace {
bool writeSnapshot(const QTemporaryDir& directory, const QByteArray& json, QString* path)
{
    *path = directory.filePath(QStringLiteral("catalog.json"));
    QFile file(*path);
    return file.open(QIODevice::WriteOnly) && file.write(json) == json.size();
}
}

TEST(OllamaClientTests, RejectsIndexOnlySnapshot)
{
    QTemporaryDir directory;
    QString path;
    ASSERT_TRUE(writeSnapshot(directory, R"({"origin":"https://ollama.com/library","revision":"r","pageCount":1,"models":[{"name":"llama","tags":["llama:1b"]}],"completeness":"pagination-terminal-verified"})", &path));
    OllamaClient client;
    EXPECT_FALSE(client.importCatalogSnapshot(QUrl::fromLocalFile(path)));
}

TEST(OllamaClientTests, RejectsMissingTagMetadata)
{
    QTemporaryDir directory;
    QString path;
    ASSERT_TRUE(writeSnapshot(directory, R"({"origin":"https://ollama.com/library","revision":"r","pageCount":1,"models":[{"name":"llama"}],"completeness":"model-and-tag-terminal-verified"})", &path));
    OllamaClient client;
    EXPECT_FALSE(client.importCatalogSnapshot(QUrl::fromLocalFile(path)));
}

TEST(OllamaClientTests, AcceptsTerminalModelAndTagSnapshot)
{
    QTemporaryDir directory;
    QString path;
    ASSERT_TRUE(writeSnapshot(directory, R"({"origin":"https://ollama.com/library","revision":"r","pageCount":1,"models":[{"name":"llama","tags":["llama:1b"]}],"completeness":"model-and-tag-terminal-verified"})", &path));
    OllamaClient client;
    EXPECT_TRUE(client.importCatalogSnapshot(QUrl::fromLocalFile(path)));
    EXPECT_EQ(client.catalogSnapshot().value(QStringLiteral("revision")).toString(), QStringLiteral("r"));
}
