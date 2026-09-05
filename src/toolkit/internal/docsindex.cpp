/*
* Audacity: A Digital Audio Editor
*/

#include "docsindex.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace au::toolkit;

namespace {
QString titleFromMarkdown(const QString& body, const QString& fallback)
{
    const QStringList lines = body.split(QStringLiteral("\n"));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("# "))) {
            return trimmed.mid(2).trimmed();
        }
    }
    return fallback;
}
}

DocsIndex::DocsIndex(QObject* parent)
    : QObject(parent)
{
    loadFromResources();
}

void DocsIndex::loadFromResources()
{
    m_articles.clear();

    QDirIterator it(QStringLiteral(":/docs/features"), QStringList() << QStringLiteral("*.md"), QDir::Files);
    while (it.hasNext()) {
        const QString path = it.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        const QString body = QString::fromUtf8(file.readAll());
        const QString id = QFileInfo(path).completeBaseName();

        QVariantMap article;
        article[QStringLiteral("id")] = id;
        article[QStringLiteral("title")] = titleFromMarkdown(body, id);
        article[QStringLiteral("body")] = body;
        m_articles << article;
    }

    emit articlesChanged();
}

QVariantList DocsIndex::articles() const
{
    return m_articles;
}

QVariantMap DocsIndex::articleById(const QString& id) const
{
    for (const QVariant& v : m_articles) {
        const QVariantMap article = v.toMap();
        if (article.value(QStringLiteral("id")).toString() == id) {
            return article;
        }
    }
    return QVariantMap();
}

QVariantList DocsIndex::search(const QString& query) const
{
    QVariantList results;
    if (query.trimmed().isEmpty()) {
        return m_articles;
    }

    for (const QVariant& v : m_articles) {
        const QVariantMap article = v.toMap();
        const QString title = article.value(QStringLiteral("title")).toString();
        const QString body = article.value(QStringLiteral("body")).toString();
        if (title.contains(query, Qt::CaseInsensitive) || body.contains(query, Qt::CaseInsensitive)) {
            results << article;
        }
    }
    return results;
}

QVariantList DocsIndex::suggestedArticles(const QString& id, int maxCount) const
{
    QVariantList suggestions;
    for (const QVariant& v : m_articles) {
        const QVariantMap article = v.toMap();
        if (article.value(QStringLiteral("id")).toString() == id) {
            continue;
        }
        suggestions << article;
        if (suggestions.size() >= maxCount) {
            break;
        }
    }
    return suggestions;
}
