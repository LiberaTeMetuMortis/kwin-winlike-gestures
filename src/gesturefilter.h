/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "gestureconfig.h"
#include "recognizer.h"

#include <input.h>

namespace WinLike
{

/**
 * Sits in KWin's input chain just before the global-shortcut filter, so it
 * sees touchpad gestures before KWin's built-in gesture bindings do.
 *
 * A gesture we handle in every direction is taken from its first event and
 * never reaches KWin. A gesture we handle in only some directions is passed
 * through until its direction is known; if that direction is ours, the rest
 * of the input chain gets a cancel event and we take over from there.
 */
class GestureFilter : public KWin::InputEventFilter
{
public:
    explicit GestureFilter(const GestureConfig *config);
    ~GestureFilter() override;

    bool swipeGestureBegin(KWin::PointerSwipeGestureBeginEvent *event) override;
    bool swipeGestureUpdate(KWin::PointerSwipeGestureUpdateEvent *event) override;
    bool swipeGestureEnd(KWin::PointerSwipeGestureEndEvent *event) override;
    bool swipeGestureCancelled(KWin::PointerSwipeGestureCancelEvent *event) override;

    bool pinchGestureBegin(KWin::PointerPinchGestureBeginEvent *event) override;
    bool pinchGestureUpdate(KWin::PointerPinchGestureUpdateEvent *event) override;
    bool pinchGestureEnd(KWin::PointerPinchGestureEndEvent *event) override;
    bool pinchGestureCancelled(KWin::PointerPinchGestureCancelEvent *event) override;

private:
    enum class Mode {
        Ignored, // not ours, pass everything through
        Watching, // passing through until the direction is known
        Owned, // ours, swallow everything
    };

    Mode modeFor(GestureType type, int fingers) const;
    void finish(GestureType type, Mode mode, int fingers, Direction direction);

    // Window switcher driven by fake Alt/Tab/arrow key presses, so KWin's
    // TabBox behaves exactly as if the user held Alt.
    void openSwitcher(std::chrono::microseconds time);
    void stepSwitcher(const QPoint &steps, std::chrono::microseconds time);
    void closeSwitcher(bool accept, std::chrono::microseconds time);

    const GestureConfig *m_config;
    SwipeRecognizer m_swipe;
    PinchRecognizer m_pinch;
    SwitcherNavigator m_navigator;
    ActionTrigger m_swipeTrigger;
    ActionTrigger m_pinchTrigger;
    bool m_switcherOpen = false;
    // Snapshot taken when the current gesture began, used until it ends.
    States m_states;
    Mode m_swipeMode = Mode::Ignored;
    Mode m_pinchMode = Mode::Ignored;
    // Set while we push our own cancel event down the chain.
    bool m_sendingCancel = false;
};

} // namespace WinLike
