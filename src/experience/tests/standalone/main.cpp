#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

#include "internal/narratorqueue.h"
#include "internal/schoolmode.h"

using namespace au::experience;

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "open record for write");
    require(file.write(data) == data.size(), "write record");
}

void testSchoolModeService()
{
    QTemporaryDir directory;
    require(directory.isValid(), "create temporary directory");
    const QString path = directory.filePath(QStringLiteral("school-mode.json"));
    SchoolModeService service(nullptr, path);

    require(service.isAvailable() && !service.isOn(), "missing record starts off and available");
    require(service.turnOn(QStringLiteral("1234")), "turn on persists atomically");
    require(service.isOn(), "enabled state visible after save");
    require(service.rename(QStringLiteral("Focus time")), "rename persists atomically");
    require(service.turnOff(QStringLiteral("1234")), "turn off validates the saved credential");
    require(!service.isOn(), "known off state visible after save");

    writeFile(path, QByteArray("{ malformed"));
    service.reload();
    require(!service.isAvailable() && !service.isOn(), "corrupt live record retains known off state");

    const QString salt = SchoolModeStore::newSaltHex();
    const QString hash = SchoolModeStore::hashCredential(QStringLiteral("5678"), salt);
    const QByteArray versionZero = QStringLiteral(
        R"({"on":true,"displayName":"Focus time","credentialHashHex":"%1","credentialSaltHex":"%2"})")
                                       .arg(hash, salt).toUtf8();
    writeFile(path, versionZero);
    service.reload();
    require(service.isAvailable() && service.isOn(), "validated version-zero record loads");
    require(service.turnOff(QStringLiteral("5678")), "version-zero credential remains usable");

    const SchoolModeStore::ParseResult persisted = SchoolModeStore::readRecordFile(path);
    require(persisted.ok && !persisted.migratedFromVersion0, "next save writes version one");
}

void testNarratorQueue()
{
    NarratorQueue queue;
    NarratorUtterance first;
    first.text = QStringLiteral("First");
    NarratorUtterance second;
    second.text = QStringLiteral("Second");
    require(queue.enqueue(first, 0), "queue first utterance");
    require(queue.enqueue(second, 10000), "queue second utterance");
    require(queue.size() == 2, "both utterances remain pending before consumer advance");
    require(queue.popNext().text == QStringLiteral("First"), "first utterance leads");
    require(queue.size() == 1, "second utterance remains pending until next advance");
    require(queue.popNext().text == QStringLiteral("Second"), "second utterance follows without overlap");
}
}

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    testSchoolModeService();
    testNarratorQueue();
    std::cout << "PASS: SchoolModeService and NarratorQueue standalone behavior\n";
    return 0;
}
