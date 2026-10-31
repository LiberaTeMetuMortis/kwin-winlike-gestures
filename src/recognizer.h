/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QPoint>
#include <QPointF>
#include <QString>

namespace WinLike
{

enum class GestureType { Swipe, Pinch, Hold };
enum class Direction { None, Up, Down, Left, Right, In, Out };

QString directionName(Direction d);
Direction opposite(Direction d);

/**
 * Accumulates the deltas of one touchpad swipe and locks onto a direction
 * once the movement passes the threshold. libinput deltas: +x right, +y down.
 */
class SwipeRecognizer
{
public:
    explicit SwipeRecognizer(qreal threshold = 30.0);

    void setThreshold(qreal threshold);
    void begin(int fingers);
    // Returns the direction on the update that locks it, None otherwise.
    Direction update(const QPointF &delta);
    void reset();

    bool active() const { return m_fingers > 0; }
    int fingers() const { return m_fingers; }
    Direction direction() const { return m_direction; }
    // Movement along the locked axis, positive in the locked direction.
    qreal progress() const;

private:
    qreal m_threshold;
    int m_fingers = 0;
    QPointF m_total;
    Direction m_direction = Direction::None;
};

/**
 * Turns continuous finger movement into discrete window-switcher steps:
 * one step per `stepDistance` units, positive = forward.
 */
class SwitcherStepper
{
public:
    explicit SwitcherStepper(qreal stepDistance = 100.0);

    void setStepDistance(qreal stepDistance);
    void reset();
    // Returns the number of steps to take for this movement (may be negative).
    int update(qreal delta);

private:
    qreal m_stepDistance;
    qreal m_pending = 0;
};

/**
 * Decides when a gesture's action runs: when the fingers lift (default), or
 * mid-swipe as soon as the fingers have travelled `distance` along the
 * locked direction.
 *
 * With `reverseUndo` (mid-swipe only), moving back by `distance` from the
 * furthest point without lifting asks for the opposite direction's action,
 * which undoes the first one (e.g. close the Overview it just opened); moving
 * forward again by `distance` from the turning point fires forward again.
 * Without it, an action fires at most once per gesture.
 */
class ActionTrigger
{
public:
    enum class Timing { OnLift, MidSwipe };
    enum class Fire { None, Forward, Reverse };

    explicit ActionTrigger(Timing timing = Timing::OnLift, qreal distance = 80.0, bool reverseUndo = false);

    void configure(Timing timing, qreal distance, bool reverseUndo = false);
    void begin();
    // Progress is travel along the locked direction. Says which action, if
    // any, should run now (MidSwipe only).
    Fire update(qreal progress);
    // Returns true when the action should run on lift.
    bool end();
    // No more fires until the next begin(), e.g. when a swipe back had
    // nothing to run: firing forward again would repeat the first action.
    void stop();

private:
    Timing m_timing;
    qreal m_distance;
    bool m_reverseUndo;
    bool m_fired = false;
    bool m_stopped = false;
    // After a forward fire we wait for a reversal, and the other way round.
    bool m_awaitingReverse = false;
    // Furthest point reached since the last fire, in the direction we're
    // waiting to move away from.
    qreal m_extreme = 0;
};

/**
 * Two-axis switcher navigation: horizontal travel becomes Left/Right steps,
 * vertical travel Up/Down steps. When one axis steps, the other axis's
 * partial travel is dropped, so the natural drift of a sideways slide never
 * adds up to an accidental up/down step (and vice versa).
 */
class SwitcherNavigator
{
public:
    explicit SwitcherNavigator(qreal horizontalStep = 100.0, qreal verticalStep = 100.0);

    void setStepDistances(qreal horizontalStep, qreal verticalStep);
    void reset();
    // x: +right / -left steps, y: +down / -up steps.
    QPoint update(const QPointF &delta);

private:
    SwitcherStepper m_x;
    SwitcherStepper m_y;
};

/**
 * Locks a pinch onto In (fingers together) or Out once the scale moves
 * far enough from 1.0.
 */
class PinchRecognizer
{
public:
    explicit PinchRecognizer(qreal threshold = 0.2);

    void setThreshold(qreal threshold);
    void begin(int fingers);
    Direction update(qreal scale);
    void reset();

    bool active() const { return m_fingers > 0; }
    int fingers() const { return m_fingers; }
    Direction direction() const { return m_direction; }

private:
    qreal m_threshold;
    int m_fingers = 0;
    Direction m_direction = Direction::None;
};

} // namespace WinLike
