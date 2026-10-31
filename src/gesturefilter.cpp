/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "gesturefilter.h"
#include "actionrunner.h"
#include "logging.h"

#include <core/inputdevice.h>
#include <effect/effecthandler.h>
#include <input_event.h>
#include <keyboard_input.h>
#include <workspace.h>

#include <linux/input-event-codes.h>

namespace WinLike
{

namespace
{

void tapKey(uint32_t key, std::chrono::microseconds time)
{
    KWin::input()->keyboard()->processKey(key, KWin::KeyboardKeyState::Pressed, time);
    KWin::input()->keyboard()->processKey(key, KWin::KeyboardKeyState::Released, time);
}

States currentStates()
{
    States states;
    // effects is null while compositing is off.
    if (KWin::effects && KWin::effects->isEffectActive(QStringLiteral("overview"))) {
        states |= State::OverviewOpen;
    }
    if (KWin::workspace()->showingDesktop()) {
        states |= State::DesktopShown;
    }
    return states;
}

std::chrono::microseconds now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
}

} // namespace

// Placed before the TabBox filter so we keep receiving swipe updates while the
// switcher we opened is on screen.
GestureFilter::GestureFilter(const GestureConfig *config)
    : KWin::InputEventFilter(KWin::InputFilterOrder::WindowSelector)
    , m_config(config)
{
}

// Never leave a fake Alt held down if we get unloaded mid-gesture.
GestureFilter::~GestureFilter()
{
    if (m_switcherOpen) {
        closeSwitcher(false, now());
    }
}

void GestureFilter::openSwitcher(std::chrono::microseconds time)
{
    qCDebug(WINLIKE) << "switcher open";
    m_navigator.setStepDistances(m_config->switcherStep(), m_config->switcherStepVertical());
    m_navigator.reset();
    m_switcherOpen = true;
    KWin::input()->keyboard()->processKey(KEY_LEFTALT, KWin::KeyboardKeyState::Pressed, time);
    tapKey(KEY_TAB, time);
}

// Arrow keys exactly as on a keyboard: TabBox moves the selection with
// Left/Right, and Up/Down (between rows in grid layouts).
void GestureFilter::stepSwitcher(const QPoint &steps, std::chrono::microseconds time)
{
    for (int i = 0; i < qAbs(steps.x()); ++i) {
        tapKey(steps.x() > 0 ? KEY_RIGHT : KEY_LEFT, time);
    }
    for (int i = 0; i < qAbs(steps.y()); ++i) {
        tapKey(steps.y() > 0 ? KEY_DOWN : KEY_UP, time);
    }
}

void GestureFilter::closeSwitcher(bool accept, std::chrono::microseconds time)
{
    qCDebug(WINLIKE) << "switcher close, accept:" << accept;
    m_switcherOpen = false;
    if (!accept) {
        tapKey(KEY_ESC, time);
    }
    KWin::input()->keyboard()->processKey(KEY_LEFTALT, KWin::KeyboardKeyState::Released, time);
}

GestureFilter::Mode GestureFilter::modeFor(GestureType type, int fingers) const
{
    if (m_config->handlesAllDirections(type, fingers, m_states)) {
        return Mode::Owned;
    }
    if (m_config->handlesAnyDirection(type, fingers, m_states)) {
        return Mode::Watching;
    }
    return Mode::Ignored;
}

void GestureFilter::finish(GestureType type, Mode mode, int fingers, Direction direction)
{
    if (mode != Mode::Owned || direction == Direction::None) {
        return;
    }
    const Action action = m_config->action(type, fingers, direction, m_states);
    qCDebug(WINLIKE) << GestureConfig::key(type, fingers, direction) << "states:" << m_states.toInt();
    runAction(action);
}

bool GestureFilter::swipeGestureBegin(KWin::PointerSwipeGestureBeginEvent *event)
{
    m_swipe.setThreshold(m_config->swipeThreshold());
    m_swipe.begin(event->fingerCount);
    m_states = currentStates();
    m_swipeMode = modeFor(GestureType::Swipe, event->fingerCount);
    m_swipeTrigger.configure(m_config->triggerTiming(), m_config->triggerDistance(), m_config->reverseUndo());
    m_swipeFired = false;
    m_swipeTrigger.begin();
    qCDebug(WINLIKE) << "swipe begin, fingers:" << event->fingerCount << "states:" << m_states.toInt();
    return m_swipeMode == Mode::Owned;
}

bool GestureFilter::swipeGestureUpdate(KWin::PointerSwipeGestureUpdateEvent *event)
{
    const Direction locked = m_swipe.update(event->delta);
    if (m_swipeMode == Mode::Watching && locked != Direction::None) {
        if (m_config->action(GestureType::Swipe, m_swipe.fingers(), locked, m_states).handled()) {
            KWin::PointerSwipeGestureCancelEvent cancel{event->time};
            m_sendingCancel = true;
            KWin::input()->processFilters(&KWin::InputEventFilter::swipeGestureCancelled, &cancel);
            m_sendingCancel = false;
            m_swipeMode = Mode::Owned;
        } else {
            m_swipeMode = Mode::Ignored;
        }
    }
    if (m_swipeMode != Mode::Owned) {
        return false;
    }

    if (locked != Direction::None) {
        if (m_config->action(GestureType::Swipe, m_swipe.fingers(), locked, m_states).kind == Action::Kind::Switcher) {
            openSwitcher(event->time);
        }
    } else if (m_switcherOpen) {
        const QPoint steps = m_navigator.update(event->delta);
        if (!steps.isNull()) {
            stepSwitcher(steps, event->time);
        }
    }
    if (!m_switcherOpen && m_swipe.direction() != Direction::None) {
        const ActionTrigger::Fire fire = m_swipeTrigger.update(m_swipe.progress());
        if (fire != ActionTrigger::Fire::None) {
            // After the first action the desktop has changed (e.g. Overview is
            // now open), so later ones look at the state as it is now: swiping
            // back after opening Overview runs "down while Overview is open".
            if (m_swipeFired) {
                m_states = currentStates();
            }
            m_swipeFired = true;
            const Direction direction = fire == ActionTrigger::Fire::Reverse ? opposite(m_swipe.direction()) : m_swipe.direction();
            finish(GestureType::Swipe, m_swipeMode, m_swipe.fingers(), direction);
        }
    }
    return true;
}

bool GestureFilter::swipeGestureEnd(KWin::PointerSwipeGestureEndEvent *event)
{
    const Mode mode = m_swipeMode;
    if (m_switcherOpen) {
        closeSwitcher(true, event->time);
    } else if (m_swipeTrigger.end()) {
        finish(GestureType::Swipe, mode, m_swipe.fingers(), m_swipe.direction());
    }
    m_swipe.reset();
    m_swipeMode = Mode::Ignored;
    return mode == Mode::Owned;
}

bool GestureFilter::swipeGestureCancelled(KWin::PointerSwipeGestureCancelEvent *event)
{
    if (m_sendingCancel) {
        return false;
    }
    qCDebug(WINLIKE) << "swipe cancelled, fingers:" << m_swipe.fingers();
    if (m_switcherOpen) {
        closeSwitcher(false, event->time);
    }
    const Mode mode = m_swipeMode;
    m_swipe.reset();
    m_swipeMode = Mode::Ignored;
    return mode == Mode::Owned;
}

bool GestureFilter::pinchGestureBegin(KWin::PointerPinchGestureBeginEvent *event)
{
    m_pinch.setThreshold(m_config->pinchThreshold());
    m_pinch.begin(event->fingerCount);
    m_states = currentStates();
    m_pinchMode = modeFor(GestureType::Pinch, event->fingerCount);
    // A pinch has no travel distance: mid-swipe timing fires once it locks.
    m_pinchTrigger.configure(m_config->triggerTiming(), 0);
    m_pinchTrigger.begin();
    qCDebug(WINLIKE) << "pinch begin, fingers:" << event->fingerCount << "states:" << m_states.toInt();
    return m_pinchMode == Mode::Owned;
}

bool GestureFilter::pinchGestureUpdate(KWin::PointerPinchGestureUpdateEvent *event)
{
    const Direction locked = m_pinch.update(event->scale);
    if (m_pinchMode == Mode::Watching && locked != Direction::None) {
        if (m_config->action(GestureType::Pinch, m_pinch.fingers(), locked, m_states).handled()) {
            KWin::PointerPinchGestureCancelEvent cancel{event->time};
            m_sendingCancel = true;
            KWin::input()->processFilters(&KWin::InputEventFilter::pinchGestureCancelled, &cancel);
            m_sendingCancel = false;
            m_pinchMode = Mode::Owned;
        } else {
            m_pinchMode = Mode::Ignored;
        }
    }
    if (m_pinchMode == Mode::Owned && m_pinch.direction() != Direction::None && m_pinchTrigger.update(1) != ActionTrigger::Fire::None) {
        finish(GestureType::Pinch, m_pinchMode, m_pinch.fingers(), m_pinch.direction());
    }
    return m_pinchMode == Mode::Owned;
}

bool GestureFilter::pinchGestureEnd(KWin::PointerPinchGestureEndEvent *)
{
    const Mode mode = m_pinchMode;
    if (m_pinchTrigger.end()) {
        finish(GestureType::Pinch, mode, m_pinch.fingers(), m_pinch.direction());
    }
    m_pinch.reset();
    m_pinchMode = Mode::Ignored;
    return mode == Mode::Owned;
}

bool GestureFilter::pinchGestureCancelled(KWin::PointerPinchGestureCancelEvent *)
{
    if (m_sendingCancel) {
        return false;
    }
    const Mode mode = m_pinchMode;
    m_pinch.reset();
    m_pinchMode = Mode::Ignored;
    return mode == Mode::Owned;
}

} // namespace WinLike
