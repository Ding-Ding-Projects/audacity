#include "buildprovenance.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <stdexcept>

using au::appshell::BuildProvenance;
static int checks = 0;
static void check(bool value, const char* message)
{
    ++checks;
    if (!value) { throw std::runtime_error(message); }
}
static QJsonObject validManifest()
{
    return {{"schemaVersion", 1}, {"version", "4.0.0"}, {"versionLabel", ""}, {"buildNumber", "14"}, {"buildId", QString(32, QLatin1Char('c'))},
            {"sourceRevision", QString(40, QLatin1Char('a'))}, {"sourceTree", QString(40, QLatin1Char('b'))},
            {"buildStartedAtUtc", "2026-09-06T23:15:37Z"}, {"timestampKind", "build-start"}};
}
static BuildProvenance parse(const QJsonObject& object)
{
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    return BuildProvenance::fromManifest(bytes, QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}
static void unavailable(const BuildProvenance& value)
{
    check(!value.isValid(), "Invalid manifest was accepted");
    check(value.version().isEmpty(), "Invalid manifest produced a version");
    check(value.updatedAtUtc().isEmpty(), "Invalid manifest produced UTC provenance");
    check(value.updatedAtLocal(QTimeZone("UTC")).isEmpty(), "Invalid manifest produced local provenance");
}
int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try {
        const auto valid = parse(validManifest());
        check(valid.isValid(), "Valid manifest rejected");
        check(valid.version() == QStringLiteral("4.0.0.14"), "Version and build number did not come from manifest");
        check(valid.updatedAtUtc() == QStringLiteral("2026-09-06 23:15:37 UTC"), "UTC formatting incorrect");
        const QTimeZone toronto("America/Toronto");
        check(toronto.isValid(), "Toronto timezone unavailable in test runtime");
        check(valid.updatedAtLocal(toronto) == QStringLiteral("2026-09-06 19:15:37 UTC-04:00 (America/Toronto)"), "Local summer timestamp or label incorrect");
        auto winter = validManifest();
        winter["buildStartedAtUtc"] = "2026-01-06T23:15:37Z";
        check(parse(winter).updatedAtLocal(toronto) == QStringLiteral("2026-01-06 18:15:37 UTC-05:00 (America/Toronto)"), "Local winter offset incorrect");
        auto offset = validManifest();
        offset["buildStartedAtUtc"] = "2026-09-07T04:45:37+05:30";
        check(parse(offset).updatedAtUtc() == valid.updatedAtUtc(), "Offset timestamp was labelled UTC without conversion");
        check(parse(offset).updatedAtLocal(toronto) == valid.updatedAtLocal(toronto), "Offset timestamp did not identify the same instant");
        check(valid.updatedAtLocal(QTimeZone("Invalid/Zone")).isEmpty(), "Invalid local timezone received a guessed value");
        auto label = validManifest();
        label["versionLabel"] = "beta";
        check(parse(label).version() == QStringLiteral("4.0.0-beta.14"), "Version label not read from manifest");
        label["buildNumber"] = "";
        check(parse(label).version() == QStringLiteral("4.0.0-beta"), "Missing optional build number left punctuation");
        for (const auto& field : validManifest().keys()) {
            auto object = validManifest(); object.remove(field); unavailable(parse(object));
        }
        for (const auto& value : { "", "unknown", "4.0", "v4.0.0", "4.0.0\n", "04.0.0" }) {
            auto object = validManifest(); object["version"] = value; unavailable(parse(object));
        }
        for (const auto& value : { "", "short", "gggggggggggggggggggggggggggggggggggggggg", "0000000000000000000000000000000000000000" }) {
            auto object = validManifest(); object["sourceRevision"] = value; unavailable(parse(object));
        }
        for (const auto& value : { "", "2026-09-06T23:15:37", "2026-02-30T23:15:37Z", "2026-09-06T25:15:37Z", "2026-09-06T23:15:37+05", "2026-09-06 23:15:37 UTC" }) {
            auto object = validManifest(); object["buildStartedAtUtc"] = value; unavailable(parse(object));
        }
        auto wrongKind = validManifest(); wrongKind["timestampKind"] = "source-commit"; unavailable(parse(wrongKind));
        auto wrongSchema = validManifest(); wrongSchema["schemaVersion"] = "1"; unavailable(parse(wrongSchema));
        auto extra = validManifest(); extra["unexpected"] = true; unavailable(parse(extra));
        const auto bytes = QJsonDocument(validManifest()).toJson();
        unavailable(BuildProvenance::fromManifest(bytes, QByteArray()));
        unavailable(BuildProvenance::fromManifest(bytes, QByteArray(64, '0')));
        unavailable(BuildProvenance::fromManifest(bytes + "changed", QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()));
        const QByteArray malformed("{");
        unavailable(BuildProvenance::fromManifest(malformed, QCryptographicHash::hash(malformed, QCryptographicHash::Sha256).toHex()));
        std::cout << "Front build provenance: " << checks << " assertions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
