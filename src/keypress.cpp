/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "keypress.h"

#include <linux/input-event-codes.h>

namespace WinLike
{

std::optional<QList<uint32_t>> modifierKeys(KeyModifiers requested, const QStringList &levelModifiers)
{
    bool shift = requested.shift;
    bool altGr = false;
    for (const QString &name : levelModifiers) {
        if (name == QLatin1String("Shift")) {
            shift = true;
        } else if (name == QLatin1String("Mod5") || name == QLatin1String("LevelThree")) {
            // ISO_Level3_Shift, which layouts put on the right Alt key (AltGr).
            altGr = true;
        } else {
            return std::nullopt;
        }
    }

    QList<uint32_t> keys;
    if (requested.ctrl) {
        keys << KEY_LEFTCTRL;
    }
    if (shift) {
        keys << KEY_LEFTSHIFT;
    }
    if (requested.alt) {
        keys << KEY_LEFTALT;
    }
    if (requested.meta) {
        keys << KEY_LEFTMETA;
    }
    if (altGr) {
        keys << KEY_RIGHTALT;
    }
    return keys;
}

} // namespace WinLike
