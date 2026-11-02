/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QStringList>

#include <cstdint>
#include <optional>

namespace WinLike
{

// Modifiers the Press key combination asks for ("Ctrl+Shift+T").
struct KeyModifiers {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool meta = false;
};

// The evdev modifier keys to hold while pressing a key, in press order: the
// ones asked for, plus those the layout needs to reach the key's level, given
// by xkb modifier name ("Shift" for '!', "Mod5"/"LevelThree" for AltGr).
// Empty optional when the level needs a modifier we have no key for.
std::optional<QList<uint32_t>> modifierKeys(KeyModifiers requested, const QStringList &levelModifiers);

} // namespace WinLike
