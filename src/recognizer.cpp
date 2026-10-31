/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recognizer.h"

#include <QtMath>

namespace WinLike
{

QString directionName(Direction d)
{
    switch (d) {
    case Direction::Up:
        return QStringLiteral("Up");
    case Direction::Down:
        return QStringLiteral("Down");
    case Direction::Left:
        return QStringLiteral("Left");
    case Direction::Right:
        return QStringLiteral("Right");
    case Direction::In:
        return QStringLiteral("In");
    case Direction::Out:
        return QStringLiteral("Out");
    case Direction::None:
        break;
    }
    return QStringLiteral("None");
}

Direction opposite(Direction d)
{
    switch (d) {
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    case Direction::Right:
        return Direction::Left;
    case Direction::In:
        return Direction::Out;
    case Direction::Out:
        return Direction::In;
    case Direction::None:
        break;
    }
    return Direction::None;
}

SwipeRecognizer::SwipeRecognizer(qreal threshold)
    : m_threshold(threshold)
{
}

void SwipeRecognizer::setThreshold(qreal threshold)
{
    m_threshold = threshold;
}

void SwipeRecognizer::begin(int fingers)
{
    reset();
    m_fingers = fingers;
}

Direction SwipeRecognizer::update(const QPointF &delta)
{
    m_total += delta;
    if (m_direction != Direction::None) {
        return Direction::None;
    }
    if (qHypot(m_total.x(), m_total.y()) < m_threshold) {
        return Direction::None;
    }
    if (qAbs(m_total.x()) >= qAbs(m_total.y())) {
        m_direction = m_total.x() > 0 ? Direction::Right : Direction::Left;
    } else {
        m_direction = m_total.y() > 0 ? Direction::Down : Direction::Up;
    }
    return m_direction;
}

void SwipeRecognizer::reset()
{
    m_fingers = 0;
    m_total = {};
    m_direction = Direction::None;
}

qreal SwipeRecognizer::progress() const
{
    switch (m_direction) {
    case Direction::Right:
        return m_total.x();
    case Direction::Left:
        return -m_total.x();
    case Direction::Down:
        return m_total.y();
    case Direction::Up:
        return -m_total.y();
    default:
        return 0;
    }
}

SwitcherStepper::SwitcherStepper(qreal stepDistance)
    : m_stepDistance(stepDistance)
{
}

void SwitcherStepper::setStepDistance(qreal stepDistance)
{
    m_stepDistance = stepDistance;
}

void SwitcherStepper::reset()
{
    m_pending = 0;
}

int SwitcherStepper::update(qreal delta)
{
    m_pending += delta;
    const int steps = static_cast<int>(m_pending / m_stepDistance);
    m_pending -= steps * m_stepDistance;
    return steps;
}

ActionTrigger::ActionTrigger(Timing timing, qreal distance, bool reverseUndo)
    : m_timing(timing)
    , m_distance(distance)
    , m_reverseUndo(reverseUndo)
{
}

void ActionTrigger::configure(Timing timing, qreal distance, bool reverseUndo)
{
    m_timing = timing;
    m_distance = distance;
    m_reverseUndo = reverseUndo;
}

void ActionTrigger::begin()
{
    m_fired = false;
    m_stopped = false;
    m_awaitingReverse = false;
    m_extreme = 0;
}

ActionTrigger::Fire ActionTrigger::update(qreal progress)
{
    if (m_timing != Timing::MidSwipe || m_stopped) {
        return Fire::None;
    }
    if (m_awaitingReverse) {
        if (!m_reverseUndo) {
            return Fire::None;
        }
        m_extreme = qMax(m_extreme, progress);
        if (progress > m_extreme - m_distance) {
            return Fire::None;
        }
        m_awaitingReverse = false;
        m_extreme = progress;
        return Fire::Reverse;
    }
    // The first fire counts from where the swipe started; later ones from
    // the point where the fingers turned around.
    if (m_fired) {
        m_extreme = qMin(m_extreme, progress);
    }
    if (progress < m_extreme + m_distance) {
        return Fire::None;
    }
    m_fired = true;
    m_awaitingReverse = true;
    m_extreme = progress;
    return Fire::Forward;
}

bool ActionTrigger::end()
{
    // Mid-swipe mode only acts once the distance is reached; a short swipe
    // that never got there does nothing.
    return m_timing == Timing::OnLift && !m_stopped;
}

void ActionTrigger::stop()
{
    m_stopped = true;
}

SwitcherNavigator::SwitcherNavigator(qreal horizontalStep, qreal verticalStep)
    : m_x(horizontalStep)
    , m_y(verticalStep)
{
}

void SwitcherNavigator::setStepDistances(qreal horizontalStep, qreal verticalStep)
{
    m_x.setStepDistance(horizontalStep);
    m_y.setStepDistance(verticalStep);
}

void SwitcherNavigator::reset()
{
    m_x.reset();
    m_y.reset();
}

QPoint SwitcherNavigator::update(const QPointF &delta)
{
    const int x = m_x.update(delta.x());
    const int y = m_y.update(delta.y());
    if (x != 0) {
        m_y.reset();
    }
    if (y != 0) {
        m_x.reset();
    }
    return {x, y};
}

PinchRecognizer::PinchRecognizer(qreal threshold)
    : m_threshold(threshold)
{
}

void PinchRecognizer::setThreshold(qreal threshold)
{
    m_threshold = threshold;
}

void PinchRecognizer::begin(int fingers)
{
    reset();
    m_fingers = fingers;
}

Direction PinchRecognizer::update(qreal scale)
{
    if (m_direction != Direction::None) {
        return Direction::None;
    }
    if (scale <= 1.0 - m_threshold) {
        m_direction = Direction::In;
    } else if (scale >= 1.0 + m_threshold) {
        m_direction = Direction::Out;
    }
    return m_direction;
}

void PinchRecognizer::reset()
{
    m_fingers = 0;
    m_direction = Direction::None;
}

} // namespace WinLike
