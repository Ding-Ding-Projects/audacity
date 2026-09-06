"""Product shape and declared source-role checks for completeness evidence."""
import ast
import re
import struct
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
from pathlib import PurePosixPath


def utc(value):
    if not isinstance(value, str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z", value):
        raise ValueError("UTC timestamp with seconds required")
    return datetime.strptime(value, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)


def source_role(role, value, ref, read_json, require, product):
    """Resolve exact role identifiers in bounded, candidate-bound source formats."""
    require(isinstance(value, dict) and set(value) == {"file", "id"}, f"{role}: role-specific file/id object required, not prose")
    identifier = value["id"]
    require(isinstance(identifier, str) and 0 < len(identifier) <= 240, f"{role}: exact role ID required")
    file = ref(value["file"], source=True)
    name = value["file"]["path"]
    text = file.read_text(encoding="utf-8")
    suffix = file.suffix
    if role == "implementation":
        require(name.startswith("src/") and suffix in (".cpp", ".h", ".qml") if product == "desktop" else name.startswith("docs/site/") and suffix in (".html", ".js", ".css"), "implementation: product-specific source root/format required")
        require(re.fullmatch(r"[A-Za-z_][A-Za-z0-9_-]*", identifier), "implementation: invalid symbol ID")
        # Exclude comments before checking exact declaration boundaries.
        code = re.sub(r"/\*.*?\*/|//[^\n]*|<!--.*?-->", "", text, flags=re.S)
        needle = re.escape(identifier)
        if suffix == ".html":
            matches = re.findall(r'\bid\s*=\s*[\"\']([^\"\']+)[\"\']', code)
            valid = identifier in matches
        elif suffix == ".css": valid = re.search(r"(?:^|[}\n])\s*[.#]" + needle + r"\s*[{,:]", code)
        else:
            valid = re.search(r"\b(?:class|struct|function)\s+" + needle + r"\b|\bid\s*:\s*" + needle + r"\b|\b" + needle + r"\s*\([^;{}]*\)\s*(?:const\s*)?(?:override\s*)?\{", code)
        require(valid, "implementation: exact declaration absent")
    elif role == "documentation":
        require(name.startswith("docs/features/") and suffix == ".md", "documentation: feature article root/format required")
        require(any(re.fullmatch(r"#{1,6}\s+" + re.escape(identifier) + r"\s*", line) for line in text.splitlines()), "documentation: exact heading absent")
    elif role == "localized":
        require("::" in identifier, "localized: context::exact-source ID required")
        context, source = identifier.split("::", 1)
        require(context and source, "localized: context and exact message required")
        if product == "desktop":
            require(name == "share/locale/audacity_yue_HK.ts", "localized: desktop catalog path required")
            tree = ET.fromstring(text)
            contexts = [c for c in tree.findall("context") if c.findtext("name") == context]
            messages = [m for c in contexts for m in c.findall("message") if m.findtext("source") == source]
            require(len(contexts) == 1 and len(messages) == 1, "localized: exact context/message absent or duplicated")
            translated = messages[0].find("translation")
            require(translated is not None and translated.get("type") not in ("unfinished", "vanished", "obsolete") and "".join(translated.itertext()).strip(), "localized: translated message absent")
        else:
            require(name.startswith("docs/site/locales/") and suffix == ".json", "localized: website catalog root/format required")
            catalog = read_json(file)
            require(catalog.get("language") == "yue" and isinstance(catalog.get("contexts", {}).get(context, {}).get(source), str) and catalog["contexts"][context][source].strip(), "localized: exact website context/message absent")
    elif role == "testSource":
        require(name.startswith(("buildscripts/checks/", "src/", "tests/")) and suffix in (".py", ".cpp", ".js"), "testSource: test root/format required")
        require(re.fullmatch(r"[A-Za-z_][A-Za-z0-9_.]*", identifier), "testSource: invalid exact test ID")
        if suffix == ".py":
            require(file.name.startswith("test_"), "testSource: test filename required")
            identifiers = {n.name for n in ast.walk(ast.parse(text)) if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef)) and n.name.startswith("test_")}
            valid = identifier in identifiers
        else:
            code = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
            if suffix == ".cpp":
                identifiers = {a + "." + b for a, b in re.findall(r"\bTEST(?:_F|_P)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)", code)}
                valid = identifier in identifiers
            else: valid = re.search(r"\b(?:test|it)\s*\(\s*['\"]" + re.escape(identifier) + r"['\"]\s*,", code)
        require(valid, "testSource: exact test declaration absent")
    elif role == "persistence":
        require(name.startswith("docs/inventory/persistence/") and suffix == ".json", "persistence: contract root/format required")
        contract = read_json(file)
        require(contract.get("schemaVersion") == 1 and contract.get("id") == identifier and contract.get("product") == product, "persistence: exact contract ID/product absent")
        require(contract.get("mode") in ("persistent", "stateless") and isinstance(contract.get("reason"), str) and contract["reason"].strip(), "persistence: explicit behavior contract required")
        if contract["mode"] == "persistent":
            require(isinstance(contract.get("location"), str) and contract["location"].strip(), "persistence: location absent")
    return file


def pe_version(data, require):
    """Read PE headers, executable entry point and RT_VERSION fixed information.

    This verifies file shape and embedded numeric version, not executability or
    authenticity. Production execution still requires the separate launch record.
    """
    require(len(data) >= 128 and data[:2] == b"MZ", "desktop artifact is not PE")
    pe = struct.unpack_from("<I", data, 60)[0]
    require(64 <= pe <= len(data) - 24 and data[pe:pe + 4] == b"PE\0\0", "desktop PE signature absent")
    machine, count, _, _, _, opt_size, flags = struct.unpack_from("<HHIIIHH", data, pe + 4)
    require(machine in (0x14c, 0x8664, 0xaa64) and 0 < count <= 96 and flags & 2 and not flags & 0x2000, "desktop PE executable headers invalid")
    opt = pe + 24
    require(opt_size >= 112 and opt + opt_size + 40 * count <= len(data), "desktop PE optional/section header invalid")
    magic = struct.unpack_from("<H", data, opt)[0]
    require(magic in (0x10b, 0x20b), "desktop PE optional magic invalid")
    dd = opt + (96 if magic == 0x10b else 112)
    require(dd + 24 <= opt + opt_size, "desktop PE resource directory absent")
    entry = struct.unpack_from("<I", data, opt + 16)[0]
    resource_rva, resource_size = struct.unpack_from("<II", data, dd + 16)
    sections = []
    for i in range(count):
        offset = opt + opt_size + 40 * i
        virtual_size, rva, size, raw = struct.unpack_from("<IIII", data, offset + 8)
        characteristics = struct.unpack_from("<I", data, offset + 36)[0]
        require(raw + size <= len(data), "desktop PE section exceeds file")
        sections.append((rva, size, raw, characteristics))
    require(any(rva <= entry < rva + size and flags & 0x20000000 for rva, size, _, flags in sections), "desktop PE executable entry point absent")
    def file_offset(rva, length):
        for start, size, raw, _ in sections:
            if start <= rva and rva + length <= start + size: return raw + rva - start
        require(False, "desktop PE resource RVA outside section")
    base = file_offset(resource_rva, resource_size)
    require(resource_size >= 16, "desktop PE version resource absent")
    def entries(relative):
        require(0 <= relative <= resource_size - 16, "desktop PE resource tree invalid")
        n = sum(struct.unpack_from("<HH", data, base + relative + 12))
        require(0 < n <= 128 and relative + 16 + n * 8 <= resource_size, "desktop PE resource entries invalid")
        return [struct.unpack_from("<II", data, base + relative + 16 + j * 8) for j in range(n)]
    roots = [target for name, target in entries(0) if name == 16]
    require(len(roots) == 1 and roots[0] & 0x80000000, "desktop PE RT_VERSION absent")
    names = entries(roots[0] & 0x7fffffff)
    require(len(names) == 1 and names[0][1] & 0x80000000, "desktop PE version names ambiguous")
    languages = entries(names[0][1] & 0x7fffffff)
    require(len(languages) == 1 and not languages[0][1] & 0x80000000, "desktop PE version languages ambiguous")
    leaf = languages[0][1]
    require(leaf + 16 <= resource_size, "desktop PE resource data entry invalid")
    rva, length = struct.unpack_from("<II", data, base + leaf)
    start = file_offset(rva, length)
    require(length >= 92, "desktop PE version value truncated")
    total, value_length, value_type = struct.unpack_from("<HHH", data, start)
    key = "VS_VERSION_INFO\0".encode("utf-16-le")
    require(total <= length and value_length == 52 and value_type == 0 and data[start + 6:start + 6 + len(key)] == key, "desktop PE fixed version header invalid")
    fixed = (start + 6 + len(key) + 3) & ~3
    require(fixed + 52 <= start + total and struct.unpack_from("<I", data, fixed)[0] == 0xfeef04bd, "desktop PE fixed version signature absent")
    high, low = struct.unpack_from("<II", data, fixed + 16)
    return [high >> 16, high & 65535, low >> 16, low & 65535]
