/*
* Audacity: A Digital Audio Editor
*/

#include "hardwarefit.h"

using namespace au::toolkit;

QString au::toolkit::hardwareFitVerdictLabel(HardwareFitVerdict verdict)
{
    switch (verdict) {
    case HardwareFitVerdict::RunsWell: return QStringLiteral("Runs well");
    case HardwareFitVerdict::RunsWithLimits: return QStringLiteral("Runs with limits");
    case HardwareFitVerdict::Unlikely: return QStringLiteral("Unlikely");
    case HardwareFitVerdict::Unknown:
    default: return QStringLiteral("Unknown");
    }
}

HardwareFitVerdict au::toolkit::computeHardwareFit(const HardwareEvidence& evidence, const ModelResourceClaim& claim)
{
    // Missing evidence on either side means the verdict cannot be honest,
    // so it stays Unknown rather than guessing from a model's name or a
    // rounded assumption.
    if (evidence.totalRamBytes <= 0 || claim.blobSizeBytes <= 0) {
        return HardwareFitVerdict::Unknown;
    }

    // A conservative rule of thumb: the model plus its working context
    // needs roughly 1.2x its blob size resident, on top of a baseline the
    // rest of the system needs to keep breathing.
    const double neededRam = static_cast<double>(claim.blobSizeBytes) * 1.2;
    const double baselineReserve = 1.5 * 1024.0 * 1024.0 * 1024.0; // 1.5 GiB
    const double availableForModel = static_cast<double>(evidence.totalRamBytes) - baselineReserve;

    if (evidence.freeDiskBytes > 0 && evidence.freeDiskBytes < claim.blobSizeBytes) {
        return HardwareFitVerdict::Unlikely;
    }

    if (evidence.vramBytes > 0) {
        if (static_cast<double>(evidence.vramBytes) >= neededRam) {
            return HardwareFitVerdict::RunsWell;
        }
        if (static_cast<double>(evidence.vramBytes) >= neededRam * 0.5 && availableForModel >= neededRam) {
            return HardwareFitVerdict::RunsWithLimits;
        }
    }

    if (availableForModel >= neededRam) {
        return HardwareFitVerdict::RunsWithLimits;
    }

    if (availableForModel > 0) {
        return HardwareFitVerdict::Unlikely;
    }

    return HardwareFitVerdict::Unlikely;
}
