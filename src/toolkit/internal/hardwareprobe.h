/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include "hardwarefit.h"

namespace au::toolkit {
//! Measures real local hardware evidence: total RAM from /proc/meminfo,
//! free disk space at a given destination path, and an nvidia-smi probe
//! for GPU VRAM when the tool is present. Nothing here is guessed; an
//! unavailable probe reports the field as unmeasured rather than assuming
//! a value.
class HardwareProbe
{
public:
    static HardwareEvidence measure(const QString& diskDestinationPath);

private:
    static qint64 measureTotalRamBytes();
    static qint64 measureFreeDiskBytes(const QString& path);
    static qint64 measureVramBytes();
};
}
