"""Exact consumer contract and independent omission mutations, no UI claims."""
import json
import re
from pathlib import Path
root = Path(__file__).resolve().parents[3]
inventory = json.loads(Path(__file__).with_name("consumer-inventory.json").read_text())
OVERLAY = "buildscripts/muse-patches/0011-isolated-profile.patch"
overlay_contracts = ["target_sources(muse_global PRIVATE ${CMAKE_SOURCE_DIR}/src/shared/profilepaths.cpp)",
    "Paths::settingsAccessed();", "m_settings->setFallbacksEnabled(false)",
    "Paths::ipcName(SERVER_NAME)", "Paths::childArguments(args)",
    "new ipc::IpcLock(au::profile::Paths::ipcName(QString::fromStdString(name)))",
    "Network requests are unavailable in an isolated verification profile.",
    "+    if (au::profile::Paths::active()) return au::profile::Paths::writableLocation(QStandardPaths::AppLocalDataLocation);"]

def code(text):
    # Preserve quoted literals while removing actual comments.
    return re.sub(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|(/\*.*?\*/|//[^\n]*)',
                  lambda m: m[1] or "", text, flags=re.S)

def guard_pattern(name):
    return re.compile(r'\b' + re.escape(name) + r'\([^;]*?\)(?: const)?\s*\{\s*(if \(au::profile::Paths::active\(\)\))', re.S)

def exact_line(text, line):
    return len(re.findall(r"(?m)^[ \t]*" + re.escape(line) + r"[ \t]*$", code(text))) == 1

def check(files):
    assert set(inventory["brandingRouteConsumers"]) == {
        "src/personalize/personalizemodule.cpp", "src/personalize/internal/brandingmodel.cpp", "src/branding/brandingstore.cpp"}
    expected_counts = {"src/personalize/personalizemodule.cpp": 3,
                       "src/personalize/internal/brandingmodel.cpp": 1,
                       "src/branding/brandingstore.cpp": 2}
    for path, boundaries in inventory["brandingRouteConsumers"].items():
        assert len(boundaries) == expected_counts[path], path
        for boundary in boundaries:
            assert exact_line(files[path], boundary), (path, boundary)
    for path, count in inventory["writableConsumers"].items():
        text = code(files[path])
        assert text.count("au::profile::Paths::writableLocation(") == count, path
        assert "QStandardPaths::writableLocation(" not in text, path
    for path, count in inventory["temporaryConsumers"].items():
        assert code(files[path]).count("au::profile::Paths::temporaryPath()") == count, path
        assert "QDir::tempPath()" not in code(files[path]), path
    for path, names in inventory["sideEffectGuards"].items():
        for name in names:
            expected = 2 if name == "Au3AudioComService::getCloudProjectPage" else 1
            assert len(list(guard_pattern(name).finditer(code(files[path])))) == expected, name
    main = code(files["src/app/main.cpp"])
    assert "au::profile::Paths::initializeArguments(profileArguments, &profileError)" in main
    assert main.index("Paths::initializeArguments") < main.index("CommandLineParser commandLineParser")
    assert "Paths::ipcName(QString::fromLatin1(appName))" in main
    assert "Paths::ipcName(QCoreApplication::applicationName())" in code(files["src/app/guiapp.cpp"])
    for item in overlay_contracts: assert item in files[OVERLAY], item

paths = set(inventory["writableConsumers"]) | set(inventory["temporaryConsumers"]) | set(inventory["sideEffectGuards"]) | set(inventory["brandingRouteConsumers"]) | {
    "src/app/main.cpp", "src/app/guiapp.cpp", OVERLAY}
files = {path:(root/path).read_text(encoding="utf-8") for path in paths}
check(files)
mutations = 0

def rejected(path, changed):
    global mutations
    broken = dict(files); broken[path] = changed
    try: check(broken)
    except AssertionError: mutations += 1
    else: raise AssertionError("omission escaped: " + path)
    check(files)

for path in paths: rejected(path, "")
for path in set(inventory["writableConsumers"]) | set(inventory["temporaryConsumers"]):
    text = files[path]
    for match in re.finditer(r'au::profile::Paths::(?:writableLocation\(|temporaryPath\(\))', text):
        rejected(path, text[:match.start()] + "disabledCall(" + text[match.end():])
for path, names in inventory["sideEffectGuards"].items():
    for name in names:
        for match in guard_pattern(name).finditer(files[path]):
            start, end = match.span(1)
            rejected(path, files[path][:start] + "if (false)" + files[path][end:])
for path, boundaries in inventory["brandingRouteConsumers"].items():
    for boundary in boundaries:
        rejected(path, files[path].replace(boundary, "removed-boundary"))
        rejected(path, files[path].replace(boundary, "// " + boundary))
        rejected(path, files[path].replace(boundary, "renamed_" + boundary))
for item in overlay_contracts:
    rejected(OVERLAY, files[OVERLAY].replace(item, "removed-boundary"))
# The hand-written inventory cannot silently lose a route or one boundary.
for path in list(inventory["brandingRouteConsumers"]):
    original = inventory["brandingRouteConsumers"].pop(path)
    try: check(files)
    except AssertionError: mutations += 1
    else: raise AssertionError("inventory route omission escaped: " + path)
    inventory["brandingRouteConsumers"][path] = original
    check(files)
    for index in range(len(original)):
        inventory["brandingRouteConsumers"][path] = original[:index] + original[index + 1:]
        try: check(files)
        except AssertionError: mutations += 1
        else: raise AssertionError("inventory boundary omission escaped: " + path)
        inventory["brandingRouteConsumers"][path] = original
        check(files)
print(f"PASS {len(paths)} explicit consumer files; {mutations} omission/disabled-boundary red/restore-green regressions")
if inventory["pendingParentConsumers"]:
    print("PENDING parent consumer substitutions: " + ", ".join(inventory["pendingParentConsumers"]))
if inventory.get("pendingNetworkConsumers"):
    print("PENDING network consumer isolation: " + ", ".join(inventory["pendingNetworkConsumers"]))
