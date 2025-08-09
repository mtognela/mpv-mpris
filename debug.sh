#!/usr/bin/env bash
PLUGIN=$(realpath zig-out/lib/libmpris.so)
MPV=$(which mpv)

if [[ ! -f "$PLUGIN" ]]; then
    echo "Plugin not found at $PLUGIN"
    exit 1
fi

if [[ ! -x "$MPV" ]]; then
    echo "mpv not found"
    exit 1
fi

echo "Starting gdb with mpv and plugin: $PLUGIN"
gdb --args "$MPV" -v --script="$PLUGIN" --no-audio-display "$@"