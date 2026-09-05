/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>

#include "hardwarefit.h"

namespace au::toolkit {
//! A thin QML-facing wrapper around HardwareProbe and computeHardwareFit.
//! Evidence is measured once when the service is created (real RAM, real
//! free disk space at the platform's generic data location, and a real
//! nvidia-smi probe for VRAM when present) and reused for every verdict
//! asked of it in the same session, so a page full of catalog rows does
//! not re-run nvidia-smi once per row.
class HardwareFitService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString evidenceSummary READ evidenceSummary CONSTANT)

public:
    explicit HardwareFitService(QObject* parent = nullptr);

    QString evidenceSummary() const;

    //! Returns one of "Runs well", "Runs with limits", "Unlikely" or
    //! "Unknown", computed only from measured evidence; a model is never
    //! judged from its name.
    Q_INVOKABLE QString verdictFor(qint64 blobSizeBytes) const;

private:
    HardwareEvidence m_evidence;
};
}
