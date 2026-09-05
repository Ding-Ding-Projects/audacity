/*
* Audacity: A Digital Audio Editor
*/

#include "hardwarefitservice.h"

#include <QStandardPaths>

#include "hardwareprobe.h"

using namespace au::toolkit;

HardwareFitService::HardwareFitService(QObject* parent)
    : QObject(parent)
{
    const QString destination = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    m_evidence = HardwareProbe::measure(destination);
}

QString HardwareFitService::evidenceSummary() const
{
    QStringList parts;
    if (m_evidence.totalRamBytes > 0) {
        parts << QStringLiteral("%1 GiB RAM").arg(m_evidence.totalRamBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    } else {
        parts << QStringLiteral("RAM unmeasured");
    }
    if (m_evidence.vramBytes > 0) {
        parts << QStringLiteral("%1 GiB VRAM").arg(m_evidence.vramBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    } else {
        parts << QStringLiteral("no GPU probe available");
    }
    if (m_evidence.freeDiskBytes > 0) {
        parts << QStringLiteral("%1 GiB free disk").arg(m_evidence.freeDiskBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    } else {
        parts << QStringLiteral("disk space unmeasured");
    }
    return parts.join(QStringLiteral(", "));
}

QString HardwareFitService::verdictFor(qint64 blobSizeBytes) const
{
    ModelResourceClaim claim;
    claim.blobSizeBytes = blobSizeBytes;
    return hardwareFitVerdictLabel(computeHardwareFit(m_evidence, claim));
}
