/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recognizer.h"

#include <QTest>

using namespace WinLike;

class TestRecognizer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Handing a swipe to KWin as a swipe in another direction: movement towards
    // the swipe's direction becomes movement towards the target, so moving back
    // still moves back, and sideways drift turns along with it.
    void turnDelta_data()
    {
        QTest::addColumn<QPointF>("delta");
        QTest::addColumn<Direction>("from");
        QTest::addColumn<Direction>("to");
        QTest::addColumn<QPointF>("expected");
        QTest::newRow("same direction") << QPointF(3, -4) << Direction::Left << Direction::Left << QPointF(3, -4);
        QTest::newRow("up as left") << QPointF(0, -10) << Direction::Up << Direction::Left << QPointF(-10, 0);
        QTest::newRow("back down as right") << QPointF(0, 10) << Direction::Up << Direction::Left << QPointF(10, 0);
        QTest::newRow("left as up") << QPointF(-10, 0) << Direction::Left << Direction::Up << QPointF(0, -10);
        QTest::newRow("up as down") << QPointF(0, -10) << Direction::Up << Direction::Down << QPointF(0, 10);
        QTest::newRow("right as left") << QPointF(10, 2) << Direction::Right << Direction::Left << QPointF(-10, -2);
        QTest::newRow("sideways drift turns too") << QPointF(4, -10) << Direction::Up << Direction::Left << QPointF(-10, -4);
    }

    void turnDelta()
    {
        QFETCH(QPointF, delta);
        QFETCH(Direction, from);
        QFETCH(Direction, to);
        QFETCH(QPointF, expected);
        const QPointF turned = WinLike::turnDelta(delta, from, to);
        QVERIFY2(qFuzzyCompare(turned.x() + 1, expected.x() + 1) && qFuzzyCompare(turned.y() + 1, expected.y() + 1),
                 qPrintable(QStringLiteral("got %1, %2").arg(turned.x()).arg(turned.y())));
    }

    void staysUnlockedBelowThreshold()
    {
        SwipeRecognizer r(30);
        r.begin(3);
        QCOMPARE(r.update({10, 0}), Direction::None);
        QCOMPARE(r.update({10, 5}), Direction::None);
        QCOMPARE(r.direction(), Direction::None);
    }

    void locksOnDominantAxis_data()
    {
        QTest::addColumn<QPointF>("delta");
        QTest::addColumn<Direction>("expected");
        QTest::newRow("right") << QPointF(12, 2) << Direction::Right;
        QTest::newRow("left") << QPointF(-12, 3) << Direction::Left;
        QTest::newRow("up") << QPointF(1, -12) << Direction::Up;
        QTest::newRow("down") << QPointF(-2, 12) << Direction::Down;
    }

    void locksOnDominantAxis()
    {
        QFETCH(QPointF, delta);
        QFETCH(Direction, expected);
        SwipeRecognizer r(30);
        r.begin(3);
        QCOMPARE(r.update(delta), Direction::None);
        QCOMPARE(r.update(delta), Direction::None);
        QCOMPARE(r.update(delta), expected);
        QCOMPARE(r.direction(), expected);
    }

    void reportsLockOnlyOnce()
    {
        SwipeRecognizer r(30);
        r.begin(4);
        QCOMPARE(r.update({40, 0}), Direction::Right);
        QCOMPARE(r.update({40, 0}), Direction::None);
        QCOMPARE(r.update({-200, 0}), Direction::None); // lock does not flip
        QCOMPARE(r.direction(), Direction::Right);
    }

    void progressAlongLockedAxis()
    {
        SwipeRecognizer r(30);
        r.begin(3);
        r.update({0, -40});
        r.update({5, -20});
        QCOMPARE(r.progress(), 60.0);
        r.update({0, 70});
        QCOMPARE(r.progress(), -10.0);
    }

    void resetClearsState()
    {
        SwipeRecognizer r(30);
        r.begin(3);
        r.update({40, 0});
        r.reset();
        QVERIFY(!r.active());
        QCOMPARE(r.direction(), Direction::None);
        r.begin(3);
        QCOMPARE(r.fingers(), 3);
        QCOMPARE(r.update({0, 40}), Direction::Down);
    }

    void switcherSteps()
    {
        SwitcherStepper s(100);
        QCOMPARE(s.update(60), 0);
        QCOMPARE(s.update(60), 1); // 120 -> one step, 20 left over
        QCOMPARE(s.update(250), 2); // 270 -> two steps, 70 left over
        QCOMPARE(s.update(-80), 0); // back to -10
        QCOMPARE(s.update(-95), -1); // -105 -> one step back
        s.reset();
        QCOMPARE(s.update(99), 0);
    }

    void navigatorBothAxes()
    {
        SwitcherNavigator n(100);
        QCOMPARE(n.update({120, 0}), QPoint(1, 0));
        QCOMPARE(n.update({0, 110}), QPoint(0, 1));
        QCOMPARE(n.update({0, -230}), QPoint(0, -2));
        QCOMPARE(n.update({-100, 0}), QPoint(-1, 0));
    }

    void navigatorDriftDoesNotStep()
    {
        SwitcherNavigator n(100);
        // A long horizontal slide drifting slightly downwards: the vertical
        // remainder is dropped every time a horizontal step fires.
        int down = 0;
        int right = 0;
        for (int i = 0; i < 10; ++i) {
            const QPoint steps = n.update({50, 15});
            right += steps.x();
            down += steps.y();
        }
        QCOMPARE(right, 5);
        QCOMPARE(down, 0);
    }

    void navigatorSeparateVerticalStep()
    {
        SwitcherNavigator n(100, 50);
        QCOMPARE(n.update({0, 60}), QPoint(0, 1));
        QCOMPARE(n.update({60, 0}), QPoint(0, 0));
        n.setStepDistances(100, 30);
        n.reset();
        QCOMPARE(n.update({0, -65}), QPoint(0, -2));
    }

    void navigatorReset()
    {
        SwitcherNavigator n(100);
        n.update({90, 90});
        n.reset();
        QCOMPARE(n.update({20, 20}), QPoint(0, 0));
    }

    void triggerOnLift()
    {
        ActionTrigger t(ActionTrigger::Timing::OnLift, 80);
        t.begin();
        QCOMPARE(t.update(500), ActionTrigger::Fire::None); // never mid-swipe
        QVERIFY(t.end());
    }

    void triggerMidSwipe()
    {
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QCOMPARE(t.update(40), ActionTrigger::Fire::None);
        QCOMPARE(t.update(85), ActionTrigger::Fire::Forward); // fires once the distance is reached...
        QCOMPARE(t.update(200), ActionTrigger::Fire::None); // ...only once...
        QVERIFY(!t.end()); // ...and not again on lift
    }

    void triggerMidSwipeShortSwipeFiresNothing()
    {
        // A short swipe that never reaches the distance does nothing, so a
        // flick can't trigger an action by accident.
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QCOMPARE(t.update(50), ActionTrigger::Fire::None);
        QVERIFY(!t.end());
    }

    void triggerRearmsOnBegin()
    {
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::Forward);
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::Forward);
    }

    void triggerNoReverseWithoutOption()
    {
        // Swiping back without "swipe back to undo" does nothing more.
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::Forward);
        QCOMPARE(t.update(-200), ActionTrigger::Fire::None);
    }

    void triggerReverseUndo()
    {
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80, true);
        t.begin();
        QCOMPARE(t.update(90), ActionTrigger::Fire::Forward);
        QCOMPARE(t.update(150), ActionTrigger::Fire::None); // keeps going: still one action
        // Coming back is measured from the furthest point (150), not where it fired.
        QCOMPARE(t.update(100), ActionTrigger::Fire::None);
        QCOMPARE(t.update(69), ActionTrigger::Fire::Reverse); // 150 - 81
        QCOMPARE(t.update(0), ActionTrigger::Fire::None);
        // ...and forward again from the lowest point (0).
        QCOMPARE(t.update(79), ActionTrigger::Fire::None);
        QCOMPARE(t.update(80), ActionTrigger::Fire::Forward);
        QVERIFY(!t.end());
    }

    void triggerReverseNeedsFullDistance()
    {
        // A small wobble after firing must not undo it.
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80, true);
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::Forward);
        QCOMPARE(t.update(60), ActionTrigger::Fire::None);
        QCOMPARE(t.update(100), ActionTrigger::Fire::None);
    }

    void triggerStop()
    {
        // When a swipe back has nothing to run, the gesture stops there:
        // moving forward again must not re-run (toggle) the first action.
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80, true);
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::Forward);
        QCOMPARE(t.update(0), ActionTrigger::Fire::Reverse);
        t.stop();
        QCOMPARE(t.update(200), ActionTrigger::Fire::None);
        QCOMPARE(t.update(-200), ActionTrigger::Fire::None);
        QVERIFY(!t.end());
        // A new gesture starts fresh.
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::Forward);
    }

    void triggerReverseIgnoredOnLift()
    {
        // The option only exists for mid-swipe timing.
        ActionTrigger t(ActionTrigger::Timing::OnLift, 80, true);
        t.begin();
        QCOMPARE(t.update(100), ActionTrigger::Fire::None);
        QCOMPARE(t.update(-100), ActionTrigger::Fire::None);
        QVERIFY(t.end());
    }

    void oppositeDirection()
    {
        QCOMPARE(opposite(Direction::Up), Direction::Down);
        QCOMPARE(opposite(Direction::Left), Direction::Right);
        QCOMPARE(opposite(Direction::In), Direction::Out);
        QCOMPARE(opposite(Direction::None), Direction::None);
    }

    void pinch()
    {
        PinchRecognizer p(0.2);
        p.begin(2);
        QCOMPARE(p.update(0.9), Direction::None);
        QCOMPARE(p.update(0.75), Direction::In);
        QCOMPARE(p.update(0.5), Direction::None);
        p.reset();
        p.begin(2);
        QCOMPARE(p.update(1.25), Direction::Out);
    }
};

QTEST_GUILESS_MAIN(TestRecognizer)
#include "test_recognizer.moc"
