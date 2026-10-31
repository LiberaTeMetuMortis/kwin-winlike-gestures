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
        QVERIFY(!t.update(500)); // never mid-swipe
        QVERIFY(t.end());
    }

    void triggerMidSwipe()
    {
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QVERIFY(!t.update(40));
        QVERIFY(t.update(85)); // fires once the distance is reached...
        QVERIFY(!t.update(200)); // ...only once...
        QVERIFY(!t.end()); // ...and not again on lift
    }

    void triggerMidSwipeShortSwipeFiresNothing()
    {
        // A short swipe that never reaches the distance does nothing, so a
        // flick can't trigger an action by accident.
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QVERIFY(!t.update(50));
        QVERIFY(!t.end());
    }

    void triggerRearmsOnBegin()
    {
        ActionTrigger t(ActionTrigger::Timing::MidSwipe, 80);
        t.begin();
        QVERIFY(t.update(100));
        t.begin();
        QVERIFY(t.update(100));
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
