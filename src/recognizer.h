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
