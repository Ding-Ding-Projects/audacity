/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "appshell/iappshellconfiguration.h"
#include "framework/global/modularity/ioc.h"

#include "internal/dimsumcatalog.h"
#include "internal/dimsumsurprise.h"
#include "internal/schoolmode.h"

namespace au::experience {
//! Backs the dim sum surprise card. Decides, once per process, whether this
//! launch shows the surprise, and exposes the chosen dish. There is no
//! property here that turns the surprise off; the only thing this model
//! offers callers is when to ask (see offerIfDue()), never whether the
//! answer can be forced to no.
class DimSumSurpriseModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool visible READ isVisible NOTIFY revealed FINAL)
    Q_PROPERTY(QString dishLabel READ dishLabel NOTIFY revealed FINAL)
    Q_PROPERTY(bool photoAvailable READ photoAvailable NOTIFY revealed FINAL)
    Q_PROPERTY(QString photoPath READ photoPath NOTIFY revealed FINAL)

    //! Reads the application's own first-launch-setup-completed flag, kept
    //! by the app shell. A machine that has not finished first launch setup
    //! yet is treated as its first run, and the surprise never offers there.
    muse::GlobalInject<au::appshell::IAppShellConfiguration> appShellConfiguration;

public:
    explicit DimSumSurpriseModel(QObject* parent = nullptr);

    //! Called once, after the window is up and no dialog or background task
    //! is active. Reads the real first-run state from the app shell
    //! configuration and never offers there. Consumes the one-shot draw.
    //! The AU_DIM_SUM_FORCE=1 environment variable forces a reveal for
    //! headless verification; it is a debug hook, never a user setting, and
    //! it still respects the first-run and School mode checks.
    Q_INVOKABLE void offerIfDue();

    bool isVisible() const { return m_visible; }
    QString dishLabel() const { return m_dish.bilingualLabel(); }
    bool photoAvailable() const { return !m_photoPath.isEmpty(); }
    QString photoPath() const { return m_photoPath; }

signals:
    void revealed();

private:
    //! True when the app shell has not yet completed its first-launch
    //! setup, or when that configuration cannot be reached at all (fails
    //! safe toward never showing the surprise on an uncertain first run).
    bool isFirstRun() const;

    DimSumDraw m_draw;
    DimSumSurpriseService m_service;
    SchoolModeStore::ParseResult m_schoolMode;
    DimSumDish m_dish;
    QString m_photoPath;
    bool m_visible = false;
};
}
