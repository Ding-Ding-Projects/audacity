/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>

namespace au::personalize {
/*!
 * \brief Exposes QrEncoder to QML as the small module matrix a Canvas can
 * paint directly, with no image codec and no network involved anywhere.
 */
class QrCodeModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int size READ size NOTIFY textChanged)
    Q_PROPERTY(bool ok READ ok NOTIFY textChanged)

public:
    explicit QrCodeModel(QObject* parent = nullptr);

    QString text() const;
    void setText(const QString& text);

    int size() const;
    bool ok() const;

    Q_INVOKABLE bool moduleAt(int x, int y) const;

signals:
    void textChanged();

private:
    QString m_text;
    int m_size = 0;
    bool m_ok = false;
    QVector<bool> m_modules;
};
}
