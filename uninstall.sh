#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
# SPDX-License-Identifier: GPL-2.0-or-later
# Remove the plugin and its settings page installed by install.sh.
# Run as your normal user (not with sudo); it asks for the password itself.
#   --purge   also delete your gesture settings (~/.config/kwinwinlikegesturesrc)
set -euo pipefail

cd "$(dirname "$0")"

purge=false
[[ ${1:-} == --purge ]] && purge=true

if [[ $EUID -eq 0 ]]; then
    echo "Run this as your normal user, not with sudo: it needs your session to talk to KWin." >&2
    exit 1
fi

if [[ ! -t 0 ]]; then
    echo "Run this in a terminal (e.g. Konsole): sudo needs one to ask for your password." >&2
    exit 1
fi

# Configure (no build needed) so CMake writes this distro's install paths.
if ! configure_log=$(cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr 2>&1); then
    echo "$configure_log" >&2
    exit 1
fi
mapfile -t files < build/installed-files.txt

sudo -v

echo "==> Turning the plugin off"
# Stop KWin from trying to load it at the next login, and unload it now.
kwriteconfig6 --file kwinrc --group Plugins --key winlikegesturesEnabled --delete
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.UnloadPlugin winlikegestures >/dev/null 2>&1 || true

echo "==> Removing files"
for f in "${files[@]}"; do
    # Removing (unlinking) is safe even while KWin still has the plugin mapped.
    if [[ -e $f ]]; then
        sudo rm -f -- "$f"
        echo "removed $f"
    fi
done

config=${XDG_CONFIG_HOME:-$HOME/.config}/kwinwinlikegesturesrc
if [[ -e $config ]]; then
    if $purge; then
        rm -f -- "$config"
        echo "removed $config"
    else
        echo "Kept your gesture settings in $config (use --purge to delete them)."
    fi
fi

echo
echo "Done. Plasma's own touchpad gestures are back now; log out and back in to fully unload the old code."
