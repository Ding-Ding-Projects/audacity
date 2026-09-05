/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QString>

namespace au::toolkit {
//! One of the four evidence backed hardware fit verdicts. The verdict is
//! never guessed from a model's name; it is computed only from measured
//! system memory, measured free disk space and the reported model size.
enum class HardwareFitVerdict {
    RunsWell,
    RunsWithLimits,
    Unlikely,
    Unknown
};

struct HardwareEvidence {
    //! Total system RAM in bytes, 0 when it could not be measured.
    qint64 totalRamBytes = 0;
    //! Free disk space at the download destination, in bytes, 0 when
    //! unmeasured.
    qint64 freeDiskBytes = 0;
    //! Best available estimate of usable VRAM in bytes, -1 when the probe
    //! found no GPU or the probe is unavailable on this host.
    qint64 vramBytes = -1;
};

struct ModelResourceClaim {
    //! Size of the model's blob on disk, in bytes, as reported by the
    //! catalog or the local installation. 0 means unknown.
    qint64 blobSizeBytes = 0;
};

QString hardwareFitVerdictLabel(HardwareFitVerdict verdict);

//! Compute a fit verdict from real measured evidence only. Missing evidence
//! (a zero RAM reading, a negative VRAM reading, a zero blob size) always
//! produces Unknown rather than a guess.
HardwareFitVerdict computeHardwareFit(const HardwareEvidence& evidence, const ModelResourceClaim& claim);
}
