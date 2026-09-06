#!/usr/bin/env bash
# Fails if the application opens the Welcome dialog or the First Launch Setup
# wizard on its own during a startup with a completely fresh profile.
#
# How it decides: it launches the built binary under Xvfb with brand new
# XDG_DATA_HOME and XDG_CONFIG_HOME directories, so every setting starts at
# its shipped default exactly as it would on a first install. It waits for
# the window to settle, then greps the application's own log for the trace
# lines that the startup code prints when it opens the "audacity://welcomedialog"
# or "audacity://firstLaunchSetup" URIs. If either line appears, startup
# nagged the user and the check fails. See docs/features/no-nagging.md.

set -u

BINARY="${AUDACITY_BINARY:-./build/linux/src/app/audacity}"
WAIT_SECONDS="${NO_NAGGING_WAIT_SECONDS:-30}"

if [ ! -x "$BINARY" ]; then
    echo "no_nagging_smoke: built binary not found at $BINARY (set AUDACITY_BINARY)" >&2
    exit 2
fi

WORKDIR="$(mktemp -d)"
trap 'kill "$APP_PID" 2>/dev/null; rm -rf "$WORKDIR"' EXIT

export XDG_DATA_HOME="$WORKDIR/data"
export XDG_CONFIG_HOME="$WORKDIR/config"
mkdir -p "$XDG_DATA_HOME" "$XDG_CONFIG_HOME"

LOG_FILE="$WORKDIR/audacity.log"

QT_QPA_PLATFORM=xcb AU_ALLOW_MULTIPLE_PROCESSES=1 \
    xvfb-run -a -s "-screen 0 1600x1000x24" \
    "$BINARY" > "$LOG_FILE" 2>&1 &
APP_PID=$!

sleep "$WAIT_SECONDS"

if ! kill -0 "$APP_PID" 2>/dev/null; then
    echo "no_nagging_smoke: the application exited before the wait finished; see $LOG_FILE" >&2
    cat "$LOG_FILE" >&2
    exit 2
fi

FAILED=0

if grep -q "audacity://welcomedialog" "$LOG_FILE"; then
    echo "no_nagging_smoke: the Welcome dialog opened on a fresh profile without the user asking for it" >&2
    FAILED=1
fi

if grep -q "audacity://firstLaunchSetup" "$LOG_FILE"; then
    echo "no_nagging_smoke: the First Launch Setup wizard opened on a fresh profile without the user asking for it" >&2
    FAILED=1
fi

kill "$APP_PID" 2>/dev/null
wait "$APP_PID" 2>/dev/null

if [ "$FAILED" -eq 0 ]; then
    echo "no_nagging_smoke: no unsolicited startup dialog was opened"
fi

exit "$FAILED"
