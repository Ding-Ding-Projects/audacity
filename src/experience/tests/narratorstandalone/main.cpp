#include <QCoreApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <iostream>
#include <limits>
#include "internal/narratorservice.h"
using namespace au::experience;
static int checks = 0;
void require(bool ok, const char* name) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
void pump(int milliseconds) {
    QElapsedTimer clock; clock.start();
    while (clock.elapsed() < milliseconds) { QCoreApplication::processEvents(); QThread::msleep(1); }
}
struct ProcessBackend : NarratorBackend {
    QProcess process;
    QString mode;
    QStringList spoken;
    QStringList selected;
    double lastRate = 0, lastPitch = 0;
    int cancellations = 0;
    QVector<NarratorVoice> inventory { { "en", "English", NarratorLanguage::English }, { "yue", "Cantonese", NarratorLanguage::Cantonese } };
    explicit ProcessBackend(QString value) : mode(value) {
        process.setStandardOutputFile(QProcess::nullDevice());
        process.setStandardErrorFile(QProcess::nullDevice());
    }
    ~ProcessBackend() override { cancel(); }
    NarratorEngineKind kind() const override { return NarratorEngineKind::ProcessBackend; }
    QVector<NarratorVoice> voices() const override { return inventory; }
    bool start(const QString& text, const QString& voice, double rate, double pitch) override {
        spoken << text; selected << voice; lastRate = rate; lastPitch = pitch;
        process.start(mode == "missing" ? "Z:/missing-narrator-engine.exe" : QCoreApplication::applicationFilePath(), { "--child", mode });
        return true;
    }
    State poll() override {
        if (process.state() != QProcess::NotRunning) return State::Speaking;
        return process.error() == QProcess::FailedToStart || process.exitStatus() == QProcess::CrashExit || process.exitCode() != 0
            ? State::Failed : State::Finished;
    }
    void cancel() override {
        ++cancellations;
        if (process.state() != QProcess::NotRunning) { process.kill(); process.waitForFinished(1000); }
    }
};
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().contains("--child")) {
        const QString mode = app.arguments().last();
        if (mode == "crash") { std::abort(); }
        if (mode == "exit") return 0;
        QTimer::singleShot(3000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    // Construct and enumerate the compiled native backend without invoking speech.
    auto native = makeNativeNarratorBackend();
#ifdef Q_OS_WIN
    require(native != nullptr, "native SAPI implementation linked");
    const auto nativeVoices = native->voices();
    for (const auto& voice : nativeVoices) require(!voice.id.isEmpty(), "native voice has persistent identifier");
    std::cout << "Native SAPI voice count: " << nativeVoices.size() << '\n';
#endif
    for (const QString mode : { QString("exit"), QString("crash"), QString("missing"), QString("hang") }) {
        auto owned = std::make_unique<ProcessBackend>(mode); auto* backend = owned.get();
        NarratorEngine engine(std::move(owned), 180);
        int finished = 0; QObject::connect(&engine, &NarratorEngine::speechFinished, [&] { ++finished; });
        engine.speak("bounded utterance", NarratorLanguage::English, "en", 0, 0, false);
        pump(500);
        require(finished == 1, "exit/crash/start-failure/hang completes exactly once");
        require(backend->process.state() == QProcess::NotRunning, "terminal process reaped");
        engine.stop(); pump(30);
        require(finished == 1, "stop after terminal cannot complete twice");
    }
    auto owned = std::make_unique<ProcessBackend>("hang"); auto* backend = owned.get();
    NarratorEngine engine(std::move(owned), 100);
    int finished = 0; QObject::connect(&engine, &NarratorEngine::speechFinished, [&] { ++finished; });
    engine.speak("cancel", NarratorLanguage::English, "en", 0, 0, false);
    engine.stop(); engine.stop(); pump(60);
    require(finished == 1, "repeated cancel completes once");
    backend->mode = "exit";
    engine.speak(QString(1500, 'a'), NarratorLanguage::English, "uninstalled", 20, std::numeric_limits<double>::quiet_NaN(), false);
    pump(200);
    require(finished == 2, "stale timeout cannot complete next generation");
    require(backend->spoken.last().size() == 1000, "engine text bound");
    require(backend->lastRate == 1 && backend->lastPitch == 0, "finite rate and pitch clamping");
    require(backend->selected.last() == "en", "missing selected voice uses same-language voice");
    require(engine.lastStatus().contains("unavailable"), "fallback status is truthful");
    backend->inventory.clear();
    engine.speak("missing", NarratorLanguage::Cantonese, "", 0, 0, false); pump(20);
    require(finished == 3 && engine.lastStatus().contains("Cantonese"), "missing language stays silent and completes");
    backend->inventory = { { "en", "English", NarratorLanguage::English }, { "yue", "Cantonese", NarratorLanguage::Cantonese } };
    require(engine.voicesFor(NarratorLanguage::Cantonese).size() == 1, "refresh after empty enumeration");
    NarratorQueue queue;
    require(queue.enqueueLocalized("Saved the project.", QStringLiteral("已儲存專案。"), NarratorLanguage::Both, NarratorCategory::Success, "save", 10000), "localized pair admitted");
    require(queue.size() == 2, "cooldown does not discard second language");
    backend->spoken.clear(); backend->selected.clear();
    QObject::connect(&engine, &NarratorEngine::speechFinished, &engine, [&] {
        if (!queue.isEmpty()) { const auto line = queue.popNext(); engine.speak(line.text, line.spokenIn, "", 0, 0, false); }
    });
    const auto first = queue.popNext(); engine.speak(first.text, first.spokenIn, "", 0, 0, false); pump(400);
    require(backend->spoken == QStringList { "Saved the project.", QStringLiteral("已儲存專案。") }, "real engine sequence carries distinct exact localized text");
    require(backend->selected == QStringList { "en", "yue" }, "English then Cantonese voices serialized");
    require(!queue.enqueueLocalized("new", "new-yue", NarratorLanguage::Both, NarratorCategory::Success, "another", 10001), "event cooldown applies once");
    {
        auto quietBackend = std::make_unique<ProcessBackend>("exit"); auto* quiet = quietBackend.get();
        NarratorEngine muted(std::move(quietBackend), 100);
        int count = 0; QObject::connect(&muted, &NarratorEngine::speechFinished, [&] { ++count; });
        muted.speak("quiet", NarratorLanguage::English, "en", 0, 0, true); pump(30);
        require(count == 1 && quiet->spoken.isEmpty(), "quiet mode completes without starting backend");
        muted.speak("not localized", NarratorLanguage::Both, "", 0, 0, false); pump(30);
        require(count == 2 && quiet->spoken.isEmpty(), "engine rejects unexpanded bilingual text");
    }
    {
        NarratorEngine absent({}, 100);
        int count = 0; QObject::connect(&absent, &NarratorEngine::speechFinished, [&] { ++count; });
        absent.speak("no engine", NarratorLanguage::English, "", 0, 0, false); absent.stop(); pump(30);
        require(count == 1, "no-engine completion and immediate stop race completes once");
    }
    {
        NarratorQueue bounded(0, 0);
        for (int i = 0; i < 40; ++i) bounded.enqueueLocalized("English", QStringLiteral("廣東話"), NarratorLanguage::Both,
            NarratorCategory::General, QString::number(i), i);
        require(bounded.size() == 64, "localized queue remains bounded");
        bounded.enqueueLocalized("Replacement", QStringLiteral("新通知"), NarratorLanguage::Both, NarratorCategory::General, "39", 41);
        require(bounded.size() == 64, "superseding pair does not grow queue");
        NarratorUtterance penultimate, last;
        while (!bounded.isEmpty()) { penultimate = last; last = bounded.popNext(); }
        require(penultimate.text == "Replacement" && last.text == QStringLiteral("新通知"), "pair supersession preserves language order");
        require(!bounded.enqueueLocalized("Untranslated", "", NarratorLanguage::Cantonese, NarratorCategory::Error, "", 50),
            "missing translation is never passed to Cantonese voice");
    }
    std::cout << "PASS: " << checks << " narrator engine assertions\n";
}

