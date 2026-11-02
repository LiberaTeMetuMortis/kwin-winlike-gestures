/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "keypress.h"

#include <QTest>

#include <linux/input-event-codes.h>

using namespace WinLike;

class TestKeyPress : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void plainKey()
    {
        QCOMPARE(modifierKeys({}, {}), (std::optional<QList<uint32_t>>{QList<uint32_t>{}}));
    }

    void requestedModifiers()
    {
        // Ctrl, Shift, Alt, Meta, in that press order.
        const KeyModifiers all{.ctrl = true, .shift = true, .alt = true, .meta = true};
        QCOMPARE(modifierKeys(all, {}),
                 (std::optional<QList<uint32_t>>{{KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_LEFTALT, KEY_LEFTMETA}}));
    }

    void shiftedLevel()
    {
        // '!' on US: the layout needs Shift for that level.
        QCOMPARE(modifierKeys({.ctrl = true}, {QStringLiteral("Shift")}),
                 (std::optional<QList<uint32_t>>{{KEY_LEFTCTRL, KEY_LEFTSHIFT}}));
        // Shift asked for and needed by the level: pressed once.
        QCOMPARE(modifierKeys({.shift = true}, {QStringLiteral("Shift")}),
                 (std::optional<QList<uint32_t>>{{KEY_LEFTSHIFT}}));
    }

    void altGrLevel()
    {
        // '@' on German: AltGr (Mod5 / LevelThree), not Shift.
        QCOMPARE(modifierKeys({}, {QStringLiteral("Mod5")}), (std::optional<QList<uint32_t>>{{KEY_RIGHTALT}}));
        QCOMPARE(modifierKeys({}, {QStringLiteral("LevelThree")}), (std::optional<QList<uint32_t>>{{KEY_RIGHTALT}}));
        QCOMPARE(modifierKeys({}, {QStringLiteral("Shift"), QStringLiteral("Mod5")}),
                 (std::optional<QList<uint32_t>>{{KEY_LEFTSHIFT, KEY_RIGHTALT}}));
    }

    void unknownLevelModifier()
    {
        // A level we can't reach with a key we know: don't guess.
        QVERIFY(!modifierKeys({}, {QStringLiteral("Mod3")}).has_value());
    }
};

QTEST_GUILESS_MAIN(TestKeyPress)
#include "test_keypress.moc"
