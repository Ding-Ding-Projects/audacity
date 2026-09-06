/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "internal/dimsumcatalog.h"
#include "internal/dimsumsurprise.h"
#include "internal/schoolmode.h"

namespace au::experience {
//! Backs the dim sum surprise card. Decides, once per process, whether this
//! launch shows the surprise, and exposes the chosen dish. There is no
//! property here that turns the surprise off; the only thing this model
//! offers callers is when to ask (see shouldOffer()), never whether the
//! answer can be forced to no.
class DimSumSurpriseModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool visible READ isVisible NOTIFY revealed FINAL)
    Q_PROPERTY(QString dishLabel READ dishLabel NOTIFY revealed FINAL)
    Q_PROPERTY(bool photoAvailable READ photoAvailable NOTIFY revealed FINAL)
    Q_PROPERTY(QString photoPath READ photoPath NOTIFY revealed FINAL)

public:
    explicit DimSumSurpriseModel(QObject* parent = nullptr);

    //! Called once, after the window is up and no dialog or background task
    //! is active, and never on the very first run. Consumes the one-shot
    //! draw. The AU_DIM_SUM_FORCE=1 environment variable forces a reveal
    //! for headless verification; it is a debug hook, never a user setting.
    Q_INVOKABLE void offerIfDue(bool isFirstRun);

    bool isVisible() const { return m_visible; }
    QString dishLabel() const { return m_dish.bilingualLabel(); }
    bool photoAvailable() const { return !m_dish.photoAsset.isEmpty(); }
    QString photoPath() const { return m_photoPath; }

signals:
    void revealed();

private:
    DimSumDraw m_draw;
    DimSumSurpriseService m_service;
    SchoolModeStore::ParseResult m_schoolMode;
    DimSumDish m_dish;
    QString m_photoPath;
    bool m_visible = false;
};
}
