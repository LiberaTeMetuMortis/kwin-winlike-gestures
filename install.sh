#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
# SPDX-License-Identifier: GPL-2.0-or-later
# Build, test and install the plugin and its settings page.
# Run as your normal user (not with sudo); it asks for the password itself.
set -euo pipefail

cd "$(dirname "$0")"

if [[ $EUID -eq 0 ]]; then
    echo "Run this as your normal user, not with sudo: it needs your session to talk to KWin." >&2
    exit 1
fi

if [[ ! -t 0 ]]; then
    echo "Run this in a terminal (e.g. Konsole): sudo needs one to ask for your password." >&2
    exit 1
fi

echo "==> Building"
# Configure output (including ECM's harmless "not a git repository" noise) is
# only shown if configuring fails.
if ! configure_log=$(cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RelWithDebInfo 2>&1); then
    echo "$configure_log" >&2
    exit 1
fi
cmake --build build -j"$(nproc)"

echo "==> Testing"
ctest --test-dir build --output-on-failure

# Written by CMake: where this distro keeps Qt plugins.
plugin_path=$(<build/plugin-path.txt)

echo "==> Installing (sudo may ask for your password)"
# Authenticate before touching anything, so a failed password never leaves
# things half-installed.
sudo -v
# CMake replaces the KWin plugin safely (new file, never overwritten in place)
# and leaves it alone when unchanged. See CMakeLists.txt.
sudo cmake --install build | grep -vE 'not a git repository|filesystem boundary' || true
# sudo leaves a root-owned manifest in our build dir; hand it back.
sudo chown "$(id -u):$(id -g)" build/install_manifest.txt

# What matters is what KWin is *running*, not what is on disk: KWin keeps a
# plugin's code from when it first loaded it, so if KWin started before the
# installed plugin file was written, it is still running older code.
kwin_pid=$(pgrep -u "$USER" -x kwin_wayland | head -n1 || true)
kwin_started=0
if [[ -n $kwin_pid ]]; then
    # Elapsed seconds rather than a start date: locale-independent.
    kwin_started=$(( $(date +%s) - $(ps -o etimes= -p "$kwin_pid") ))
fi
plugin_installed=$(stat -c %Y "$plugin_path")

echo
if (( kwin_started < plugin_installed )); then
    echo "Done. Log out and back in so KWin loads the new plugin."
else
    echo "Done. KWin already runs this plugin; only the settings page may have changed (no re-login needed)."
fi
