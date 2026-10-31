/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "gestureconfig.h"

#include <QTest>

using namespace WinLike;

class TestGestureConfig : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseAction()
    {
        QCOMPARE(Action::parse(QStringLiteral("shortcut:kwin/Show Desktop")),
                 (Action{Action::Kind::Shortcut, QStringLiteral("kwin"), QStringLiteral("Show Desktop")}));
        QCOMPARE(Action::parse(QStringLiteral("command:konsole --new-tab")),
                 (Action{Action::Kind::Command, {}, QStringLiteral("konsole --new-tab")}));
        QCOMPARE(Action::parse(QStringLiteral("block")).kind, Action::Kind::Block);
        QCOMPARE(Action::parse(QStringLiteral("switcher")).kind, Action::Kind::Switcher);
        QCOMPARE(Action::parse(QStringLiteral("passthrough")).kind, Action::Kind::Passthrough);
        QCOMPARE(Action::parse(QString()).kind, Action::Kind::Passthrough);
        // Malformed specs fall back to passthrough rather than eating gestures.
        QCOMPARE(Action::parse(QStringLiteral("shortcut:noslash")).kind, Action::Kind::Passthrough);
        QCOMPARE(Action::parse(QStringLiteral("bogus")).kind, Action::Kind::Passthrough);
    }

    void roundTrip()
    {
        for (const QString &spec : GestureConfig::defaults()) {
            QCOMPARE(Action::parse(spec).toString(), spec);
        }
    }

    void keys()
    {
        QCOMPARE(GestureConfig::key(GestureType::Swipe, 3, Direction::Up), QStringLiteral("Swipe3Up"));
        QCOMPARE(GestureConfig::key(GestureType::Pinch, 2, Direction::In), QStringLiteral("Pinch2In"));
    }

    void windowsDefaults()
    {
        GestureConfig c;
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Up).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down).name, QStringLiteral("Show Desktop"));
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left).kind, Action::Kind::Switcher);
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Right).kind, Action::Kind::Switcher);
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Up).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Left).name, QStringLiteral("Switch to Next Desktop"));
        QVERIFY(!c.action(GestureType::Pinch, 2, Direction::In).handled());
        QVERIFY(!c.action(GestureType::Swipe, 5, Direction::Up).handled());
        QCOMPARE(c.switcherStep(), 100.0);
        QCOMPARE(c.switcherStepVertical(), 60.0);
    }

    void verticalSwitcherStepOverride()
    {
        GestureConfig c;
        c.load({{QStringLiteral("SwitcherStepVertical"), QStringLiteral("35")}});
        QCOMPARE(c.switcherStepVertical(), 35.0);
        QCOMPARE(c.switcherStep(), 100.0);
        c.load({{QStringLiteral("SwitcherStepVertical"), QStringLiteral("0")}});
        QCOMPARE(c.switcherStepVertical(), 60.0);
    }

    void directionCoverage()
    {
        GestureConfig c;
        QVERIFY(c.handlesAllDirections(GestureType::Swipe, 3));
        QVERIFY(c.handlesAllDirections(GestureType::Swipe, 4));
        QVERIFY(!c.handlesAnyDirection(GestureType::Pinch, 2));
        c.load({{QStringLiteral("Swipe4Left"), QStringLiteral("passthrough")}});
        QVERIFY(!c.handlesAllDirections(GestureType::Swipe, 4));
        QVERIFY(c.handlesAnyDirection(GestureType::Swipe, 4));
    }

    void overrides()
    {
        GestureConfig c;
        c.load({
            {QStringLiteral("Swipe3Left"), QStringLiteral("command:dolphin")},
            {QStringLiteral("Swipe3Up"), QStringLiteral("passthrough")},
            {QStringLiteral("Pinch3In"), QStringLiteral("block")},
            {QStringLiteral("Threshold"), QStringLiteral("50")},
            {QStringLiteral("PinchThreshold"), QStringLiteral("0.3")},
        });
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left).kind, Action::Kind::Command);
        QVERIFY(!c.action(GestureType::Swipe, 3, Direction::Up).handled());
        QVERIFY(!c.handlesAllDirections(GestureType::Swipe, 3));
        QCOMPARE(c.action(GestureType::Pinch, 3, Direction::In).kind, Action::Kind::Block);
        QCOMPARE(c.swipeThreshold(), 50.0);
        QCOMPARE(c.pinchThreshold(), 0.3);
        // Untouched keys keep their defaults.
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down).name, QStringLiteral("Show Desktop"));
    }

    void reloadResetsToDefaults()
    {
        GestureConfig c;
        c.load({{QStringLiteral("Swipe3Up"), QStringLiteral("block")}, {QStringLiteral("Threshold"), QStringLiteral("80")}});
        c.load({});
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Up).name, QStringLiteral("Overview"));
        QCOMPARE(c.swipeThreshold(), 30.0);
    }

    void stateDefaults()
    {
        GestureConfig c;
        // Overview open: swipe down closes it (the Overview shortcut toggles).
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down, State::OverviewOpen).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Down, State::OverviewOpen).name, QStringLiteral("Overview"));
        // Desktop shown: swipe up brings the windows back.
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Up, State::DesktopShown).name, QStringLiteral("Show Desktop"));
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Up, State::DesktopShown).name, QStringLiteral("Show Desktop"));
        // No state binding: falls back to the normal one.
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Up, State::OverviewOpen).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left, State::DesktopShown).kind, Action::Kind::Switcher);
        // Normal state ignores state bindings.
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down).name, QStringLiteral("Show Desktop"));
    }

    void stateOverrides()
    {
        GestureConfig c;
        c.load({
            {QStringLiteral("Swipe3Left.Overview"), QStringLiteral("block")},
            {QStringLiteral("Swipe3Down.Overview"), QStringLiteral("passthrough")},
        });
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left, State::OverviewOpen).kind, Action::Kind::Block);
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left).kind, Action::Kind::Switcher);
        // Coverage follows the state: with Overview open, 3-down is no longer ours.
        QVERIFY(c.handlesAllDirections(GestureType::Swipe, 3));
        QVERIFY(!c.handlesAllDirections(GestureType::Swipe, 3, State::OverviewOpen));
    }

    void inheritRemovesBinding()
    {
        GestureConfig c;
        // "inherit" on a state key drops the built-in state binding,
        // so the normal one applies again.
        c.load({{QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")}});
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down, State::OverviewOpen).name, QStringLiteral("Show Desktop"));
        // On a plain key it means "no binding", i.e. passthrough.
        c.load({{QStringLiteral("Swipe3Up"), QStringLiteral("inherit")}});
        QVERIFY(!c.action(GestureType::Swipe, 3, Direction::Up).handled());
    }

    void overviewWinsOverDesktop()
    {
        GestureConfig c;
        c.load({
            {QStringLiteral("Swipe3Left.Overview"), QStringLiteral("command:a")},
            {QStringLiteral("Swipe3Left.Desktop"), QStringLiteral("command:b")},
        });
        const States both = State::OverviewOpen | State::DesktopShown;
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left, both).name, QStringLiteral("a"));
    }

    void badThresholdIgnored()
    {
        GestureConfig c;
        c.load({{QStringLiteral("Threshold"), QStringLiteral("abc")}, {QStringLiteral("PinchThreshold"), QStringLiteral("-1")}});
        QCOMPARE(c.swipeThreshold(), 30.0);
        QCOMPARE(c.pinchThreshold(), 0.2);
    }
};

QTEST_GUILESS_MAIN(TestGestureConfig)
#include "test_gestureconfig.moc"
