"""Exact consumer contract and independent omission mutations, no UI claims."""
import json
from pathlib import Path
root = Path(__file__).resolve().parents[3]
inventory = json.loads(Path(__file__).with_name("consumer-inventory.json").read_text())

def check(files):
    for path, count in inventory["writableConsumers"].items():
        text = files[path]
        assert text.count("au::profile::Paths::writableLocation(") == count, path
        assert "QStandardPaths::writableLocation(" not in text, path
    for path in inventory["temporaryConsumers"]:
        assert files[path].count("au::profile::Paths::temporaryPath()") == 1, path
        assert "QDir::tempPath()" not in files[path], path
    assert "au::profile::Paths::initializeArguments(profileArguments, &profileError)" in files["src/app/main.cpp"]
    assert files["src/app/main.cpp"].index("Paths::initializeArguments") < files["src/app/main.cpp"].index("CommandLineParser commandLineParser")
    assert "Paths::ipcName(QString::fromLatin1(appName))" in files["src/app/main.cpp"]
    assert "Paths::ipcName(QCoreApplication::applicationName())" in files["src/app/guiapp.cpp"]
    assert "if (au::profile::Paths::active()) return;" in files["src/project/internal/platform/windows/windowsrecentfilescontroller.cpp"]
    assert "if (au::profile::Paths::active()) return false;" in files["src/au3cloud/internal/platform/win/customschemeregistrar.cpp"]
    overlay = files["buildscripts/muse-patches/0011-isolated-profile.patch"]
    for item in ["target_sources(muse_global PRIVATE ${CMAKE_SOURCE_DIR}/src/shared/profilepaths.cpp)", "Paths::settingsAccessed();", "m_settings->setFallbacksEnabled(false)", "Paths::ipcName(SERVER_NAME)", "Paths::childArguments(args)", "Network requests are unavailable in an isolated verification profile."]:
        assert item in overlay, item

paths = set(inventory["writableConsumers"]) | set(inventory["temporaryConsumers"]) | {
    "src/app/main.cpp", "src/app/guiapp.cpp", "src/project/internal/platform/windows/windowsrecentfilescontroller.cpp",
    "src/au3cloud/internal/platform/win/customschemeregistrar.cpp", "buildscripts/muse-patches/0011-isolated-profile.patch"}
files = {path:(root/path).read_text(encoding="utf-8") for path in paths}
check(files)
mutations = 0
for path, text in files.items():
    broken = dict(files); broken[path] = ""
    try: check(broken)
    except AssertionError: mutations += 1
    else: raise AssertionError("omission escaped: " + path)
    check(files)
print(f"PASS {len(paths)} explicit consumers; {mutations} omitted-file red/restore-green regressions")
print("PENDING parent consumer substitutions: " + ", ".join(inventory["pendingParentConsumers"]))
