#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Material Audacity: configure and build on Linux.
#
# Usage:
#   ./build.sh          Configure and build into build/linux.
#   ./build.sh -s       Silent mode: log to file, print warnings and errors.
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build.linux"
INSTALL_DIR="${ROOT}/build.install"
SILENT=0

while [ $# -gt 0 ]; do
    case "$1" in
        -s|--silent) SILENT=1 ;;
        -h|--help) sed -n '2,10p' "$0"; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
    shift
done

echo "=== Material Audacity build"

for tool in cmake ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool was not found. Install it and try again." >&2
        exit 1
    fi
done

mkdir -p "${BUILD_DIR}"

echo
echo "=== Configure"
configure_args=(
    -S "${ROOT}"
    -B "${BUILD_DIR}"
    -G Ninja
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
    -DMUSE_ENABLE_UNIT_TESTS=OFF
)

if [ "${SILENT}" = "1" ]; then
    cmake "${configure_args[@]}" > "${BUILD_DIR}/configure.log" 2>&1 || {
        echo "ERROR: configure failed. See ${BUILD_DIR}/configure.log" >&2
        tail -n 40 "${BUILD_DIR}/configure.log" >&2
        exit 1
    }
else
    cmake "${configure_args[@]}"
fi

echo
echo "=== Build"
if [ "${SILENT}" = "1" ]; then
    cmake --build "${BUILD_DIR}" > "${BUILD_DIR}/build.log" 2>&1 || {
        echo "ERROR: build failed. Last lines of ${BUILD_DIR}/build.log:" >&2
        tail -n 40 "${BUILD_DIR}/build.log" >&2
        exit 1
    }
    grep -Ei "warning|error" "${BUILD_DIR}/build.log" || true
else
    cmake --build "${BUILD_DIR}"
fi

echo
echo "=== Install"
cmake --install "${BUILD_DIR}" > /dev/null

echo
echo "=== Build finished"
echo "Build tree:   ${BUILD_DIR}"
echo "Install tree: ${INSTALL_DIR}"
