/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace au::toolkit {
//! Reads the bundled feature documentation articles out of the module's own
//! Qt resources (packed into the qrc at build time from docs/features), and
//! provides simple title/body search over them for the in-app documentation
//! browser.
class DocsIndex : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList articles READ articles NOTIFY articlesChanged)

public:
    explicit DocsIndex(QObject* parent = nullptr);

    QVariantList articles() const;

    Q_INVOKABLE QVariantMap articleById(const QString& id) const;
    Q_INVOKABLE QVariantList search(const QString& query) const;
    Q_INVOKABLE QVariantList suggestedArticles(const QString& id, int maxCount = 3) const;

signals:
    void articlesChanged();

private:
    void loadFromResources();

    QVariantList m_articles;
};
}
