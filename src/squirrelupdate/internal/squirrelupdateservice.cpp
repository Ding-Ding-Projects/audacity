/*
* Audacity: A Digital Audio Editor
*/
#include "squirrelupdateservice.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include "packageverifier.h"
#include "releasesparser.h"
#include "squirrelinstalllayout.h"

#include "log.h"

using namespace muse;

namespace au::squirrelupdate {
namespace {
//! The first check waits a little so it never competes with start up work.
constexpr int FIRST_CHECK_DELAY_MS = 30 * 1000;
}

SquirrelUpdateService::SquirrelUpdateService(QObject* parent)
    : QObject(parent)
{
}

SquirrelUpdateService::~SquirrelUpdateService() = default;

void SquirrelUpdateService::init()
{
#ifdef Q_OS_WIN
    if (!isSupported()) {
        LOGI() << "Not a Squirrel.Windows installation, the feed checker stays off";
        return;
    }

    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    connect(m_timer, &QTimer::timeout, this, [this]() { checkForUpdate(); });

    configuration()->changed().onNotify(this, [this]() { restartTimer(); });

    restartTimer();

    QTimer::singleShot(FIRST_CHECK_DELAY_MS, this, [this]() { checkForUpdate(); });
#else
    // Squirrel.Windows is a Windows only installer. The module still builds
    // everywhere so the parser, the verifier and their tests stay portable,
    // but nothing here ever runs and the banner never appears.
    LOGI() << "The Squirrel feed checker is disabled on this platform";
#endif
}

void SquirrelUpdateService::restartTimer()
{
    if (!m_timer) {
        return;
    }

    if (!configuration()->isEnabled()) {
        m_timer->stop();
        return;
    }

    const int hours = configuration()->checkIntervalHours();
    m_timer->start(hours * 60 * 60 * 1000);
}

bool SquirrelUpdateService::isSupported() const
{
#ifdef Q_OS_WIN
    return !installedVersion().isEmpty() && QFileInfo::exists(updaterPath());
#else
    return false;
#endif
}

QString SquirrelUpdateService::installedVersion() const
{
    return SquirrelInstallLayout::versionFromPath(QCoreApplication::applicationDirPath());
}

QString SquirrelUpdateService::updaterPath() const
{
    const QString root = SquirrelInstallLayout::rootDirFromPath(QCoreApplication::applicationDirPath());
    if (root.isEmpty()) {
        return QString();
    }

    return root + "/Update.exe";
}

QString SquirrelUpdateService::squirrelTempDir() const
{
#ifdef Q_OS_WIN
    const QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!localAppData.isEmpty()) {
        return localAppData + "/SquirrelTemp";
    }
#endif
    return QDir::tempPath() + "/SquirrelTemp";
}

bool SquirrelUpdateService::isChecking() const
{
    return m_checking;
}

AvailableUpdate SquirrelUpdateService::availableUpdate() const
{
    if (m_dismissed) {
        return AvailableUpdate();
    }
    return m_available;
}

async::Notification SquirrelUpdateService::availableUpdateChanged() const
{
    return m_availableUpdateChanged;
}

QString SquirrelUpdateService::lastError() const
{
    return m_lastError;
}

void SquirrelUpdateService::dismiss()
{
    if (m_dismissed) {
        return;
    }

    m_dismissed = true;
    m_availableUpdateChanged.notify();
}

void SquirrelUpdateService::checkForUpdate()
{
    if (!isSupported() || !configuration()->isEnabled() || m_checking) {
        return;
    }

    const QUrl url(configuration()->feedUrl());
    if (!url.isValid() || url.scheme().isEmpty()) {
        finishWithError(QString("The update feed URL is not valid: %1").arg(configuration()->feedUrl()));
        return;
    }

    if (!m_networkManager) {
        m_networkManager = networkManagerCreator()->makeNetworkManager();
    }

    m_checking = true;
    m_lastError.clear();

    auto buffer = std::make_shared<QBuffer>();
    const RetVal<Progress> progress = m_networkManager->get(url, buffer);
    if (!progress.ret) {
        finishWithError(QString::fromStdString(progress.ret.toString()));
        return;
    }

    progress.val.finished().onReceive(this, [this, buffer](const ProgressResult& res) {
        if (!res.ret) {
            finishWithError(QString::fromStdString(res.ret.toString()));
            return;
        }

        onFeedDownloaded(buffer->data());
    });
}

void SquirrelUpdateService::onFeedDownloaded(const QByteArray& feed)
{
    QStringList errors;
    const ReleaseEntryList entries = ReleasesParser::parse(QString::fromUtf8(feed), &errors);
    for (const QString& error : errors) {
        LOGW() << "Skipped a malformed RELEASES entry, " << error.toStdString();
    }

    if (entries.isEmpty()) {
        finishWithError("The update feed holds no usable release entries.");
        return;
    }

    const ReleaseEntry entry = ReleasesParser::newestAfter(entries, installedVersion());
    if (!entry.isValid()) {
        LOGI() << "No newer release than " << installedVersion().toStdString();
        m_checking = false;
        return;
    }

    downloadPackage(entry);
}

void SquirrelUpdateService::downloadPackage(const ReleaseEntry& entry)
{
    const QUrl feed(configuration()->feedUrl());
    const QUrl packageUrl = feed.resolved(QUrl(entry.fileName));

    const QString tempDir = squirrelTempDir();
    if (!QDir().mkpath(tempDir)) {
        finishWithError(QString("The download folder could not be created: %1").arg(tempDir));
        return;
    }

    const QString target = tempDir + "/" + QFileInfo(entry.fileName).fileName();

    // A package left over from an earlier run is reused when it still verifies,
    // which saves downloading the same bytes twice.
    QString verifyError;
    if (PackageVerifier::verify(target, entry, &verifyError)) {
        LOGI() << "Reusing the already verified package at " << target.toStdString();
        m_available = { true, entry.version, entry.fileName, entry.isDelta, target };
        m_dismissed = false;
        m_checking = false;
        m_availableUpdateChanged.notify();
        return;
    }

    auto buffer = std::make_shared<QBuffer>();
    const RetVal<Progress> progress = m_networkManager->get(packageUrl, buffer);
    if (!progress.ret) {
        finishWithError(QString::fromStdString(progress.ret.toString()));
        return;
    }

    progress.val.finished().onReceive(this, [this, entry, buffer](const ProgressResult& res) {
        if (!res.ret) {
            finishWithError(QString::fromStdString(res.ret.toString()));
            return;
        }

        onPackageDownloaded(entry, buffer->data());
    });
}

void SquirrelUpdateService::onPackageDownloaded(const ReleaseEntry& entry, const QByteArray& bytes)
{
    const QString target = squirrelTempDir() + "/" + QFileInfo(entry.fileName).fileName();

    QFile file(target);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        finishWithError(QString("The downloaded package could not be written to %1").arg(target));
        return;
    }
    file.write(bytes);
    file.close();

    QString verifyError;
    if (!PackageVerifier::verify(target, entry, &verifyError)) {
        // A package that does not match the feed is never offered.
        QFile::remove(target);
        finishWithError(QString("The downloaded update failed its integrity check. %1").arg(verifyError));
        return;
    }

    LOGI() << "Verified update package " << entry.fileName.toStdString()
           << " for version " << entry.version.toStdString();

    m_available = { true, entry.version, entry.fileName, entry.isDelta, target };
    m_dismissed = false;
    m_checking = false;
    m_availableUpdateChanged.notify();
}

void SquirrelUpdateService::finishWithError(const QString& error)
{
    m_checking = false;
    m_lastError = error;
    LOGE() << error.toStdString();
    m_availableUpdateChanged.notify();
}

Ret SquirrelUpdateService::restartToUpdate()
{
    if (!isSupported()) {
        return make_ret(Ret::Code::NotSupported, "Squirrel.Windows updates are available on Windows only");
    }

    const AvailableUpdate update = availableUpdate();
    if (!update.available) {
        return make_ret(Ret::Code::UnknownError, "There is no verified update to apply");
    }

    const QString updater = updaterPath();
    if (updater.isEmpty() || !QFileInfo::exists(updater)) {
        return make_ret(Ret::Code::UnknownError, "Update.exe was not found beside the installation");
    }

    // Squirrel takes the directory that holds RELEASES, not the RELEASES file.
    QUrl feed(configuration()->feedUrl());
    QString feedDir = feed.toString();
    const int slash = feedDir.lastIndexOf('/');
    if (slash > 0) {
        feedDir = feedDir.left(slash);
    }

    QProcess applyProcess;
    applyProcess.start(updater, { "--update=" + feedDir });
    if (!applyProcess.waitForStarted()) {
        return make_ret(Ret::Code::UnknownError, "Update.exe could not be started");
    }

    // Squirrel applies the package it already downloaded into SquirrelTemp, so
    // this finishes quickly. The wait is bounded so a stuck updater cannot
    // freeze the application for ever.
    if (!applyProcess.waitForFinished(10 * 60 * 1000)) {
        applyProcess.kill();
        return make_ret(Ret::Code::UnknownError, "Update.exe did not finish in time");
    }

    if (applyProcess.exitCode() != 0) {
        return make_ret(Ret::Code::UnknownError, "Update.exe reported a failure");
    }

    // The shortcut launcher sits at the package root, which is what
    // --processStart expects to find.
    if (!QProcess::startDetached(updater, { "--processStart", "MaterialAudacity.exe" })) {
        return make_ret(Ret::Code::UnknownError, "The updated application could not be started");
    }

    QCoreApplication::quit();

    return make_ok();
}
}
