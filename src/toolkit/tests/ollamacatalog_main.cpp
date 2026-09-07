/* Real client tests with isolated INI settings, no GUI and no network calls. */
#include "ollamaclient.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <vector>
using au::toolkit::OllamaClient;
namespace {
const QString Origin = QStringLiteral("https://ollama.com/library");
void require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
QByteArray encoded(const QJsonObject& value) { return QJsonDocument(value).toJson(QJsonDocument::Compact); }
QString digest(const QByteArray& bytes) { return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()); }
QJsonObject metadata() {
    return {{"schemaVersion", 1}, {"origin", Origin}, {"revision", "local-r1"}, {"pageCount", 1},
        {"pages", QJsonArray{QJsonObject{{"url", Origin}, {"sha256", QString(64, 'a')}}}},
        {"models", QJsonArray{QJsonObject{{"name", "llama3.2"}, {"tags", QJsonArray{"llama3.2:1b", "llama3.2:3b"}}}}}};
}
void page(QJsonObject& root, const QString& key, QJsonValue value) {
    auto pages = root["pages"].toArray(); auto first = pages[0].toObject(); first.insert(key, value); pages[0] = first; root["pages"] = pages;
}
void model(QJsonObject& root, const QString& key, QJsonValue value) {
    auto models = root["models"].toArray(); auto first = models[0].toObject(); first.insert(key, value); models[0] = first; root["models"] = models;
}
void claim(QJsonObject& root, const QString& key, QJsonValue value) {
    auto object = root["acquisition"].toObject(); object.insert(key, value); root["acquisition"] = object;
}
QJsonObject receipt() {
    auto result = metadata();
    result["completeness"] = "observed-page-chains-terminal";
    const QJsonObject details{{"index", 1}, {"bytes", 10}, {"items", QJsonArray{"llama3.2"}}, {"next", QJsonValue::Null}, {"terminal", true}};
    for (auto it = details.begin(); it != details.end(); ++it) page(result, it.key(), it.value());
    model(result, "tagPages", QJsonArray{QJsonObject{{"index", 1}, {"url", Origin + "/llama3.2/tags"},
        {"sha256", QString(64, 'b')}, {"bytes", 20}, {"items", QJsonArray{"llama3.2:1b", "llama3.2:3b"}}, {"next", QJsonValue::Null}, {"terminal", true}}});
    result["acquisition"] = QJsonObject{{"method", "bounded-https-library-pages"}, {"startedAt", "2026-09-06T00:00:00Z"},
        {"completedAt", "2026-09-06T00:00:01Z"}, {"firstUrl", Origin}, {"terminalUrl", Origin},
        {"pageCount", 2}, {"responseBytes", 30}, {"modelCount", 1}, {"tagCount", 2}};
    result["revision"] = digest(encoded(QJsonObject{{"models", result["models"]}, {"pages", result["pages"]}}));
    return result;
}
struct Fixture {
    QTemporaryDir dir;
    OllamaClient client;
    int rejected = 0, changed = 0;
    QString path;
    Fixture() {
        require(dir.isValid(), "owned fixture directory"); path = dir.filePath("catalog.json");
        QObject::connect(&client, &OllamaClient::requestFailed, &client, [&](const QString& operation, const QString& reason) {
            require(operation == "import catalog snapshot" && !reason.isEmpty() && reason.size() < 256, "bounded specific operation diagnostic"); ++rejected;
        });
        QObject::connect(&client, &OllamaClient::catalogSnapshotChanged, &client, [&] { ++changed; });
    }
    bool load(const QByteArray& bytes) {
        { QFile file(path); require(file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size(), "write owned input"); }
        const bool accepted = client.importCatalogSnapshot(QUrl::fromLocalFile(path));
        QFile file(path); require(file.open(QIODevice::ReadOnly) && file.readAll() == bytes, "original import bytes unchanged");
        return accepted;
    }
    void reject(const QByteArray& bytes) {
        require(load(encoded(metadata())), "establish valid prior metadata");
        const auto prior = client.catalogSnapshot(); const int priorChanged = changed, priorRejected = rejected;
        require(!load(bytes), "hostile snapshot is rejected");
        require(rejected == priorRejected + 1 && changed == priorChanged && client.catalogSnapshot() == prior, "rejection signals once and preserves memory");
        QSettings settings; settings.sync(); require(settings.value("ollama/catalogSnapshot").toMap() == prior, "rejection preserves persisted metadata");
    }
};
}
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir settingsRoot;
    if (!settingsRoot.isValid()) return 2;
    QCoreApplication::setOrganizationName("AudacityCatalogFixture"); QCoreApplication::setApplicationName("CatalogValidation");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsRoot.path() + "/user");
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsRoot.path() + "/system");
    using Test = std::pair<QString, std::function<void()>>;
    std::vector<Test> tests;
    auto reject = [&](const QString& name, const std::function<void(QJsonObject&)>& mutate, bool acquired = false) {
        tests.emplace_back(name, [=] { auto root = acquired ? receipt() : metadata(); mutate(root); Fixture fixture; fixture.reject(encoded(root)); });
    };
    tests.emplace_back("metadata needs no verified-looking claim and always remains untrusted", [] {
        Fixture fixture; require(fixture.load(encoded(metadata())), "bounded local metadata accepted");
        const auto value = fixture.client.catalogSnapshot();
        require(value.value("completeness") == "untrusted-local-import" && !value.value("originVerified").toBool()
            && value.value("receiptKind") == "metadata-only" && fixture.changed == 1 && fixture.rejected == 0, "explicit local trust state");
        OllamaClient restored; require(restored.catalogSnapshot() == value, "safe metadata survives restart without acquiring trust");
    });
    tests.emplace_back("self-consistent fabricated acquisition receipts never establish official origin", [] {
        Fixture fixture; require(fixture.load(encoded(receipt())), "structurally consistent receipt claims accepted");
        const auto value = fixture.client.catalogSnapshot();
        require(value.value("completeness") == "untrusted-local-import" && !value.value("originVerified").toBool()
            && value.value("receiptKind") == "acquisition-receipt-claims"
            && value.value("models").toList().first().toMap().value("tagPages").toList().size() == 1, "receipt evidence retained without provenance promotion");
        OllamaClient restored; require(restored.catalogSnapshot() == value, "receipt claims survive restart still untrusted");
    });
    tests.emplace_back("unknown nested and claimed trust fields are not persisted", [] {
        auto root = metadata(); root["verified"] = true; root["originVerified"] = true;
        root["unknown"] = QJsonObject{{"nested", QJsonObject{{"value", "synthetic marker"}}}};
        root["completeness"] = "model-and-tag-terminal-verified"; page(root, "unknown", "synthetic"); model(root, "unknown", 42);
        Fixture fixture; require(fixture.load(encoded(root)), "unknown bounded fields can be discarded");
        const auto value = fixture.client.catalogSnapshot();
        require(!value.contains("unknown") && !value.contains("verified") && !value.value("originVerified").toBool()
            && !value.value("pages").toList().first().toMap().contains("unknown")
            && !value.value("models").toList().first().toMap().contains("unknown")
            && value.value("declaredCompleteness") == "model-and-tag-terminal-verified", "only allowlisted claims persist");
    });
    for (const QString& bad : QStringList{"https://ollama.com/library.evil.example/page", "https://ollama.com/library-attacker/page",
         "https://ollama.com/library@evil.example/page", "https://ollama.com.evil.example/library", "https://ollama.com@evil.example/library",
         "https://user@ollama.com/library", "https://@ollama.com/library", "https://ollama.com:443/library", "http://ollama.com/library",
         "https://ollama.com/library#fragment", "https://ollama.com/library#", "https://ollama.com/library?",
         "https://ollama.com/library/../blog", "https://ollama.com/library/a/../b", "https://ollama.com/library%2fother",
         "https://ollama.com/library?page=1&page=2", "https://ollama.com/library?page=0", "https://ollama.com/library?page=1001",
         "https://ollama.com/library?q=filtered", "https://ollama.com/library?sort=unknown", "https://ollama.com/library?page=%31"})
        reject("URL boundary: " + bad, [=](auto& root) { page(root, "url", bad); });
    reject("overlong page URL", [](auto& root) { page(root, "url", Origin + "/" + QString(2048, 'a')); });
    for (const QChar control : {QChar(0), QChar(0x1f), QChar(0x7f), QChar(0x202e)}) {
        reject(QStringLiteral("revision control U+%1").arg(quint32(control.unicode()), 4, 16, QLatin1Char('0')), [=](auto& root) { root["revision"] = "r" + QString(control); });
        reject(QStringLiteral("URL control U+%1").arg(quint32(control.unicode()), 4, 16, QLatin1Char('0')), [=](auto& root) { page(root, "url", Origin + QString(control)); });
    }
    for (const QString& key : QStringList{"schemaVersion", "origin", "revision", "pageCount", "pages", "models"})
        reject("missing required " + key, [=](auto& root) { root.remove(key); });
    for (const QJsonValue& value : {QJsonValue(true), QJsonValue("1"), QJsonValue(1.5), QJsonValue(QJsonValue::Null)})
        reject("page count scalar type " + QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact)), [=](auto& root) { root["pageCount"] = value; });
    reject("schema version is not a boolean", [](auto& root) { root["schemaVersion"] = true; });
    reject("non-string tag", [](auto& root) { model(root, "tags", QJsonArray{42}); });
    reject("non-object model", [](auto& root) { root["models"] = QJsonArray{"llama3.2"}; });
    reject("non-object page", [](auto& root) { root["pages"] = QJsonArray{true}; });
    reject("tag belongs to another model", [](auto& root) { model(root, "tags", QJsonArray{"other:1b"}); });
    reject("duplicate model", [](auto& root) { auto values = root["models"].toArray(); values.append(values.first()); root["models"] = values; });
    reject("duplicate tag", [](auto& root) { model(root, "tags", QJsonArray{"llama3.2:1b", "llama3.2:1b"}); });
    reject("duplicate page receipt", [](auto& root) { auto values = root["pages"].toArray(); values.append(values.first()); root["pages"] = values; root["pageCount"] = 2; });
    reject("duplicate page URL with different hash", [](auto& root) { auto values = root["pages"].toArray(); auto second = values.first().toObject(); second["sha256"] = QString(64, 'b'); values.append(second); root["pages"] = values; root["pageCount"] = 2; });
    reject("duplicate page hash with different URL", [](auto& root) { auto values = root["pages"].toArray(); auto second = values.first().toObject(); second["url"] = Origin + "?page=2"; values.append(second); root["pages"] = values; root["pageCount"] = 2; });
    reject("malformed page hash", [](auto& root) { page(root, "sha256", QString(64, 'z')); });
    reject("page count mismatch", [](auto& root) { root["pageCount"] = 2; });
    reject("10001 models", [](auto& root) { QJsonArray models; for (int i = 0; i < 10001; ++i) { const QString name = "m" + QString::number(i); models.append(QJsonObject{{"name", name}, {"tags", QJsonArray{name + ":1"}}}); } root["models"] = models; });
    reject("10001 tags per model", [](auto& root) { QJsonArray tags; for (int i = 0; i < 10001; ++i) tags.append("llama3.2:t" + QString::number(i)); model(root, "tags", tags); });
    reject("100001 aggregate tags", [](auto& root) { QJsonArray models; for (int i = 0; i < 11; ++i) { const QString name = "m" + QString::number(i); QJsonArray tags; for (int t = 0; t < (i == 10 ? 1 : 10000); ++t) tags.append(name + ":t" + QString::number(t)); models.append(QJsonObject{{"name", name}, {"tags", tags}}); } root["models"] = models; });
    reject("1001 page count", [](auto& root) { root["pageCount"] = 1001; });
    for (const QString& key : QStringList{"index", "items", "bytes", "terminal", "next"})
        reject("receipt missing " + key, [=](auto& root) { auto values = root["pages"].toArray(); auto first = values.first().toObject(); first.remove(key); values[0] = first; root["pages"] = values; }, true);
    reject("receipt missing first identity", [](auto& root) { claim(root, "firstUrl", Origin + "?page=2"); }, true);
    reject("receipt missing terminal identity", [](auto& root) { claim(root, "terminalUrl", Origin + "?page=2"); }, true);
    reject("receipt broken cursor transition", [](auto& root) { page(root, "next", Origin + "?page=2"); }, true);
    reject("receipt starts at page two", [](auto& root) { page(root, "url", Origin + "?page=2"); }, true);
    reject("receipt terminal is not a boolean", [](auto& root) { page(root, "terminal", "true"); }, true);
    reject("receipt fractional byte count", [](auto& root) { page(root, "bytes", 1.5); }, true);
    reject("receipt revision not bound to graph", [](auto& root) { root["revision"] = QString(64, 'f'); }, true);
    reject("receipt index items do not match models", [](auto& root) { page(root, "items", QJsonArray{"other"}); }, true);
    reject("receipt missing tag pages", [](auto& root) { model(root, "tagPages", QJsonArray{}); }, true);
    reject("receipt aggregate counts disagree", [](auto& root) { claim(root, "tagCount", 3); }, true);
    reject("receipt has invalid time", [](auto& root) { claim(root, "startedAt", "not-a-time"); }, true);
    reject("receipt ends before it starts", [](auto& root) { claim(root, "completedAt", "2026-09-05T00:00:00Z"); }, true);
    tests.emplace_back("duplicate escaped object keys are refused before collapse", [] {
        auto bytes = encoded(metadata()); bytes.replace("\"revision\":\"local-r1\"", "\"revision\":\"local-r1\",\"\\u0072evision\":\"changed\""); Fixture fixture; fixture.reject(bytes);
    });
    tests.emplace_back("deep unknown data is rejected before parsing", [] {
        QByteArray nested = "null"; for (int i = 0; i < 20; ++i) nested = '[' + nested + ']';
        auto bytes = encoded(metadata()); bytes.chop(1); bytes += ",\"unknown\":" + nested + '}'; Fixture fixture; fixture.reject(bytes);
    });
    tests.emplace_back("overlong unknown scalar is rejected", [] { auto root = metadata(); root["unknown"] = QString(8193, 'x'); Fixture fixture; fixture.reject(encoded(root)); });
    tests.emplace_back("16 MiB plus one input is rejected", [] { Fixture fixture; fixture.reject(QByteArray(16 * 1024 * 1024 + 1, ' ')); });
    tests.emplace_back("trailing malformed JSON is rejected", [] { Fixture fixture; fixture.reject(encoded(metadata()) + "garbage"); });
    tests.emplace_back("non-object JSON root is rejected", [] { Fixture fixture; fixture.reject('[' + encoded(metadata()) + ']'); });
    tests.emplace_back("local URL authorities queries fragments and relative files are rejected", [] {
        Fixture fixture; require(fixture.load(encoded(metadata())), "valid file baseline"); const auto prior = fixture.client.catalogSnapshot();
        QUrl query = QUrl::fromLocalFile(fixture.path); query.setQuery("x=1"); QUrl fragment = QUrl::fromLocalFile(fixture.path); fragment.setFragment("x");
        for (const QUrl& url : {QUrl("https://example.invalid/catalog.json"), QUrl("file://example.invalid/share/catalog.json"), QUrl("file:relative.json"), query, fragment, QUrl::fromLocalFile(fixture.dir.path())}) {
            const int before = fixture.rejected;
            require(!fixture.client.importCatalogSnapshot(url) && fixture.rejected == before + 1 && fixture.client.catalogSnapshot() == prior, "invalid local URL refused without state mutation");
        }
    });
    tests.emplace_back("legacy saved verified-looking metadata is ignored without deletion", [] {
        QSettings settings; QVariantMap legacy{{"origin", Origin}, {"completeness", "verified"}}; settings.setValue("ollama/catalogSnapshot", legacy); settings.sync();
        OllamaClient restored; require(restored.catalogSnapshot().isEmpty() && settings.value("ollama/catalogSnapshot").toMap() == legacy, "legacy record not promoted or destroyed");
    });
    int failures = 0;
    for (const auto& test : tests) {
        QSettings settings; settings.clear(); settings.sync();
        try { test.second(); std::fprintf(stderr, "PASS %s\n", test.first.toUtf8().constData()); }
        catch (const std::exception& error) { ++failures; std::fprintf(stderr, "FAIL %s: %s\n", test.first.toUtf8().constData(), error.what()); }
    }
    std::fprintf(stderr, "Ollama catalog: %zu cases, %zu passed, %d failed\n", tests.size(), tests.size() - failures, failures);
    return failures ? 1 : 0;
}
