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

    void parsePlasmaAction()
    {
        const Action a = Action::parse(QStringLiteral(" plasma:3Up "));
        QCOMPARE(a.kind, Action::Kind::Plasma);
        QCOMPARE(a.targetFingers, 3);
        QCOMPARE(a.targetDirection, Direction::Up);
        QCOMPARE(a.toString(), QStringLiteral("plasma:3Up"));
        QVERIFY(a.handled());
        // Plasma's own gesture already snaps back, nothing for us to undo.
        QVERIFY(!a.canUndo());
    }

    void parsePlasmaActionRejectsGesturesKWinDoesNotHave()
    {
        // KWin only has 3- and 4-finger touchpad swipes.
        for (const char *spec : {"plasma", "plasma:", "plasma:Left", "plasma:2Left", "plasma:5Left", "plasma:4In", "plasma:4Sideways", "plasma:44Left"}) {
            QCOMPARE(Action::parse(QString::fromLatin1(spec)).kind, Action::Kind::Passthrough);
        }
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
        // Plasma's own desktop slide, following the fingers: the desktop moves
        // with them, so swiping left brings in the desktop on the right.
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Left), Action::parse(QStringLiteral("plasma:4Left")));
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Right), Action::parse(QStringLiteral("plasma:4Right")));
        QVERIFY(!c.action(GestureType::Pinch, 2, Direction::In).handled());
        QVERIFY(!c.action(GestureType::Swipe, 5, Direction::Up).handled());
        QCOMPARE(c.switcherStep(), 100.0);
        QCOMPARE(c.switcherStepVertical(), 60.0);
    }

    void triggerTiming()
    {
        GestureConfig c;
        QCOMPARE(c.triggerTiming(), ActionTrigger::Timing::OnLift);
        QCOMPARE(c.triggerDistance(), 80.0);
        c.load({{QStringLiteral("TriggerTiming"), QStringLiteral("midswipe")}, {QStringLiteral("TriggerDistance"), QStringLiteral("120")}});
        QCOMPARE(c.triggerTiming(), ActionTrigger::Timing::MidSwipe);
        QCOMPARE(c.triggerDistance(), 120.0);
        // Unknown values fall back to the default.
        c.load({{QStringLiteral("TriggerTiming"), QStringLiteral("sometimes")}});
        QCOMPARE(c.triggerTiming(), ActionTrigger::Timing::OnLift);
    }

    void reverseUndo()
    {
        GestureConfig c;
        QVERIFY(!c.reverseUndo());
        c.load({{QStringLiteral("ReverseUndo"), QStringLiteral("true")}});
        QVERIFY(c.reverseUndo());
        c.load({{QStringLiteral("ReverseUndo"), QStringLiteral("false")}});
        QVERIFY(!c.reverseUndo());
        c.load({{QStringLiteral("ReverseUndo"), QStringLiteral("maybe")}});
        QVERIFY(!c.reverseUndo());
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

    void overviewIgnoresDesktopRules()
    {
        // Overview is drawn on top of a shown desktop, so while it is open a
        // missing .Overview rule falls back to the normal key, never to .Desktop.
        GestureConfig c;
        const States both = State::OverviewOpen | State::DesktopShown;
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Up, both).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down, both).name, QStringLiteral("Overview"));
        c.load({{QStringLiteral("Swipe3Left.Desktop"), QStringLiteral("command:b")}});
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left, both).kind, Action::Kind::Switcher);
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left, State::DesktopShown).name, QStringLiteral("b"));
    }

    void gridDefaults()
    {
        GestureConfig c;
        const auto grid = [&](int f, Direction d) {
            return c.action(GestureType::Swipe, f, d, State::GridOpen);
        };
        // 3 left/right move the grid's highlight, 3 down opens it, 4 down closes the grid.
        QCOMPARE(grid(3, Direction::Left).kind, Action::Kind::Key);
        QCOMPARE(grid(3, Direction::Left).name, QStringLiteral("Left"));
        QCOMPARE(grid(3, Direction::Right).name, QStringLiteral("Right"));
        QCOMPARE(grid(3, Direction::Down).name, QStringLiteral("Return"));
        QCOMPARE(grid(4, Direction::Down).name, QStringLiteral("Grid View"));
        // No .Grid rule: the normal key.
        QCOMPARE(grid(3, Direction::Up).name, QStringLiteral("Overview"));
        // All four 3-finger directions are ours in the grid, so the switcher never opens there.
        QVERIFY(c.handlesAllDirections(GestureType::Swipe, 3, State::GridOpen));
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left).kind, Action::Kind::Switcher);
        // .Grid rules don't leak into the other states.
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down, State::OverviewOpen).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down).name, QStringLiteral("Show Desktop"));
    }

    void gridIgnoresOverviewAndDesktopRules()
    {
        // The grid is drawn on top of everything: while it is open only .Grid
        // rules apply, else the normal key, never .Overview or .Desktop.
        GestureConfig c;
        c.load({
            {QStringLiteral("Swipe3Up.Overview"), QStringLiteral("command:a")},
            {QStringLiteral("Swipe3Up.Desktop"), QStringLiteral("command:b")},
            {QStringLiteral("Swipe4Left.Grid"), QStringLiteral("command:c")},
        });
        const States all = State::GridOpen | State::OverviewOpen | State::DesktopShown;
        // 3 up has no .Grid rule: the normal key, not .Overview or .Desktop.
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Up, all).name, QStringLiteral("Overview"));
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Left, all).name, QStringLiteral("c"));
        QCOMPARE(c.action(GestureType::Swipe, 4, Direction::Left, State::OverviewOpen).kind, Action::Kind::Plasma);
    }

    void gridOverridesAndInherit()
    {
        GestureConfig c;
        c.load({{QStringLiteral("Swipe3Down.Grid"), QStringLiteral("passthrough")}});
        QVERIFY(!c.handlesAllDirections(GestureType::Swipe, 3, State::GridOpen));
        QVERIFY(c.handlesAllDirections(GestureType::Swipe, 3, State::OverviewOpen));
        c.load({{QStringLiteral("Swipe3Down.Grid"), QStringLiteral("inherit")}});
        QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Down, State::GridOpen).name, QStringLiteral("Show Desktop"));
        // A default .Grid rule written back unchanged is dropped from overrides.
        QVERIFY(GestureConfig::overrides({{QStringLiteral("Swipe4Down.Grid"), QStringLiteral("shortcut:kwin/Grid View")}}).isEmpty());
    }

    void parseKey()
    {
        const Action a = Action::parse(QStringLiteral("key:Ctrl+Shift+T"));
        QCOMPARE(a.kind, Action::Kind::Key);
        QCOMPARE(a.name, QStringLiteral("Ctrl+Shift+T"));
        QCOMPARE(a.toString(), QStringLiteral("key:Ctrl+Shift+T"));
        QVERIFY(a.handled());
        QCOMPARE(Action::parse(QStringLiteral(" key:Left ")).name, QStringLiteral("Left"));
        // Empty key: nothing to press, treat as passthrough.
        QVERIFY(!Action::parse(QStringLiteral("key:")).handled());
    }

    void keyCannotUndo()
    {
        QVERIFY(!Action::parse(QStringLiteral("key:Left")).canUndo());
    }

    void ruleKeysDefaults()
    {
        QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Grid"), {}),
                 (QStringList{QStringLiteral("Swipe3Down.Grid"),
                              QStringLiteral("Swipe3Left.Grid"),
                              QStringLiteral("Swipe3Right.Grid"),
                              QStringLiteral("Swipe4Down.Grid")}));
        QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Overview"), {}).size(), 2);
        QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Desktop"), {}).size(), 2);
        QCOMPARE(GestureConfig::situations(), (QStringList{QStringLiteral(".Grid"), QStringLiteral(".Overview"), QStringLiteral(".Desktop")}));
        QCOMPARE(GestureConfig::gestureKeys().size(), 12);
        QCOMPARE(GestureConfig::gestureKeys().first(), QStringLiteral("Swipe3Up"));
        QCOMPARE(GestureConfig::gestureKeys().last(), QStringLiteral("Pinch3Out"));
    }

    void ruleKeysOverrides()
    {
        const QMap<QString, QString> o{{QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")},
                                       {QStringLiteral("Pinch2In.Overview"), QStringLiteral("block")}};
        QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Overview"), o),
                 (QStringList{QStringLiteral("Swipe4Down.Overview"), QStringLiteral("Pinch2In.Overview")}));
    }

    void ruleEqualToNormalIsKept()
    {
        // A rule doing the same as the normal gesture is still a rule.
        const QMap<QString, QString> e{{QStringLiteral("Swipe3Left.Overview"), QStringLiteral("switcher")}};
        QCOMPARE(GestureConfig::overrides(e), e);
        QVERIFY(GestureConfig::ruleKeys(QStringLiteral(".Overview"), e).contains(QStringLiteral("Swipe3Left.Overview")));
    }

    void overridesDropsStrayInherit()
    {
        const QMap<QString, QString> e{{QStringLiteral("Swipe3Left.Overview"), QStringLiteral("inherit")},
                                       {QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")}};
        // Built-in rule deleted: kept. No built-in rule to delete: meaningless, dropped.
        QCOMPARE(GestureConfig::overrides(e),
                 (QMap<QString, QString>{{QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")}}));
    }

    void canReverseNeedsBothSides()
    {
        // Swiping back undoes the action that fired with the opposite one.
        // Both must be undoable: a keystroke can't be taken back, so after
        // Press key the swipe stops instead of running (or repeating) anything.
        const Action shortcut = Action::parse(QStringLiteral("shortcut:kwin/Overview"));
        const Action key = Action::parse(QStringLiteral("key:Return"));
        const Action switcher = Action::parse(QStringLiteral("switcher"));
        QVERIFY(GestureConfig::canReverse(shortcut, shortcut));
        QVERIFY(!GestureConfig::canReverse(key, shortcut));
        QVERIFY(!GestureConfig::canReverse(shortcut, key));
        QVERIFY(!GestureConfig::canReverse(shortcut, switcher));
    }

    void canUndo()
    {
        // Only actions that run something can undo a swipe.
        QVERIFY(Action::parse(QStringLiteral("shortcut:kwin/Overview")).canUndo());
        QVERIFY(Action::parse(QStringLiteral("command:foo")).canUndo());
        QVERIFY(!Action::parse(QStringLiteral("switcher")).canUndo());
        QVERIFY(!Action::parse(QStringLiteral("block")).canUndo());
        QVERIFY(!Action::parse(QStringLiteral("passthrough")).canUndo());
    }

    void overridesKeepsDeliberateStateKeys()
    {
        // A state key equal to its normal gesture was written on purpose: it
        // must survive, or changing the normal gesture later changes it too.
        const QMap<QString, QString> entries = {
            {QStringLiteral("Swipe4Left.Overview"), QStringLiteral("shortcut:kwin/Switch to Previous Desktop")},
            {QStringLiteral("Swipe3Up"), QStringLiteral("command:foo")},
            {QStringLiteral("Swipe3Up.Overview"), QStringLiteral("command:foo")},
            // Equal to built-in defaults: dropped.
            {QStringLiteral("Swipe3Down"), QStringLiteral("shortcut:kwin/Show Desktop")},
            {QStringLiteral("Swipe3Down.Overview"), QStringLiteral("shortcut:kwin/Overview")},
            {QStringLiteral("Threshold"), QStringLiteral("30")},
            {QStringLiteral("ReverseUndo"), QStringLiteral("false")},
        };
        const QMap<QString, QString> expected = {
            {QStringLiteral("Swipe4Left.Overview"), QStringLiteral("shortcut:kwin/Switch to Previous Desktop")},
            {QStringLiteral("Swipe3Up"), QStringLiteral("command:foo")},
            {QStringLiteral("Swipe3Up.Overview"), QStringLiteral("command:foo")},
        };
        QCOMPARE(GestureConfig::overrides(entries), expected);
    }

    void tuningDefaultsMatchLoad()
    {
        // The settings page compares against tuningDefaults(); they must be
        // what load() falls back to.
        GestureConfig c;
        c.load({});
        const QMap<QString, QString> t = GestureConfig::tuningDefaults();
        QCOMPARE(t.value(QStringLiteral("Threshold")).toDouble(), c.swipeThreshold());
        QCOMPARE(t.value(QStringLiteral("PinchThreshold")).toDouble(), c.pinchThreshold());
        QCOMPARE(t.value(QStringLiteral("SwitcherStep")).toDouble(), c.switcherStep());
        QCOMPARE(t.value(QStringLiteral("SwitcherStepVertical")).toDouble(), c.switcherStepVertical());
        QCOMPARE(t.value(QStringLiteral("TriggerTiming")), QStringLiteral("lift"));
        QCOMPARE(c.triggerTiming(), ActionTrigger::Timing::OnLift);
        QCOMPARE(t.value(QStringLiteral("TriggerDistance")).toDouble(), c.triggerDistance());
        QCOMPARE(t.value(QStringLiteral("ReverseUndo")), QStringLiteral("false"));
        QVERIFY(!c.reverseUndo());
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
