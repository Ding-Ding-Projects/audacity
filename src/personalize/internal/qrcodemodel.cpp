/*
* Audacity: A Digital Audio Editor
*/

#include "qrcodemodel.h"

#include "qrencoder.h"

using namespace au::personalize;

QrCodeModel::QrCodeModel(QObject* parent)
    : QObject(parent)
{
}

QString QrCodeModel::text() const
{
    return m_text;
}

void QrCodeModel::setText(const QString& text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;

    QrEncoder::Result result = QrEncoder::encode(text.toUtf8());
    m_ok = result.ok;
    m_size = result.size;
    m_modules = result.modules;

    emit textChanged();
}

int QrCodeModel::size() const
{
    return m_size;
}

bool QrCodeModel::ok() const
{
    return m_ok;
}

bool QrCodeModel::moduleAt(int x, int y) const
{
    if (!m_ok || x < 0 || y < 0 || x >= m_size || y >= m_size) {
        return false;
    }
    return m_modules.at(y * m_size + x);
}
