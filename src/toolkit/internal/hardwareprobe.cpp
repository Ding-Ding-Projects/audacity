#include "shared/profilepaths.h"
/*
* Audacity: A Digital Audio Editor
*/

#include "hardwareprobe.h"

#include <QFile>
#include <QDir>
#include <QStorageInfo>
#include <QProcess>
#include <QStandardPaths>

using namespace au::toolkit;

qint64 HardwareProbe::measureTotalRamBytes()
{
#if defined(Q_OS_LINUX)
    QFile meminfo(QStringLiteral("/proc/meminfo"));
    if (!meminfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    const QByteArray content = meminfo.readAll();
    const QByteArrayList lines = content.split('\n');
    for (const QByteArray& line : lines) {
        if (line.startsWith("MemTotal:")) {
            const QByteArrayList parts = line.split(' ');
            for (const QByteArray& part : parts) {
                bool ok = false;
                const qint64 kib = part.toLongLong(&ok);
                if (ok && kib > 0) {
                    return kib * 1024;
                }
            }
        }
    }
    return 0;
#else
    return 0;
#endif
}

qint64 HardwareProbe::measureFreeDiskBytes(const QString& path)
{
    QStorageInfo info(path.isEmpty() ? au::profile::Paths::writableLocation(QStandardPaths::HomeLocation) : path);
    if (!info.isValid()) {
        return 0;
    }
    return info.bytesAvailable();
}

qint64 HardwareProbe::measureVramBytes()
{
    if (au::profile::Paths::active()) return -1;
    const QString nvidiaSmi = QStandardPaths::findExecutable(QStringLiteral("nvidia-smi"));
    if (nvidiaSmi.isEmpty()) {
        return -1;
    }

    QProcess process;
    process.start(nvidiaSmi, { QStringLiteral("--query-gpu=memory.total"), QStringLiteral("--format=csv,noheader,nounits") });
    if (!process.waitForFinished(3000)) {
        process.kill();
        return -1;
    }
    if (process.exitCode() != 0) {
        return -1;
    }

    const QByteArray output = process.readAllStandardOutput().trimmed();
    const QByteArrayList lines = output.split('\n');
    if (lines.isEmpty()) {
        return -1;
    }
    bool ok = false;
    const qint64 megabytes = lines.first().trimmed().toLongLong(&ok);
    if (!ok || megabytes <= 0) {
        return -1;
    }
    return megabytes * 1024 * 1024;
}

HardwareEvidence HardwareProbe::measure(const QString& diskDestinationPath)
{
    if (au::profile::Paths::active()) return HardwareEvidence {};
    HardwareEvidence evidence;
    evidence.totalRamBytes = measureTotalRamBytes();
    evidence.freeDiskBytes = measureFreeDiskBytes(diskDestinationPath);
    evidence.vramBytes = measureVramBytes();
    return evidence;
}
