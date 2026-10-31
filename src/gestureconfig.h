/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "recognizer.h"

#include <QFlags>
#include <QHash>
#include <QMap>
#include <QString>

namespace WinLike
{

// Desktop state at the moment a gesture starts. A binding keyed with a state
// suffix ("Swipe3Down.Overview") wins over the plain one while that state holds.
enum class State {
    OverviewOpen = 1 << 0, // suffix ".Overview"
    DesktopShown = 1 << 1, // suffix ".Desktop"
};
Q_DECLARE_FLAGS(States, State)
Q_DECLARE_OPERATORS_FOR_FLAGS(States)

struct Action {
    enum class Kind {
        Passthrough, // let KWin / the focused app handle the gesture as usual
        Block, // swallow the gesture and do nothing
        Shortcut, // invoke a kglobalaccel shortcut: component + name
        Command, // run a shell command
        Switcher, // hold the Alt+Tab switcher open, step with finger movement
    };

    Kind kind = Kind::Passthrough;
    QString component;
    QString name; // shortcut name, or the command line

    static Action parse(const QString &spec);
    QString toString() const;
    bool handled() const { return kind != Kind::Passthrough; }
    bool operator==(const Action &) const = default;
};

/**
 * Gesture bindings, keyed like "Swipe3Up" or "Pinch2In".
 * Built-in defaults follow Windows precision-touchpad behavior; values from
 * the user's config file override them one key at a time.
 */
class GestureConfig
{
public:
    GestureConfig();

    static QString key(GestureType type, int fingers, Direction direction);
    static QMap<QString, QString> defaults();

    // Applies overrides. Keys: "Threshold", "PinchThreshold", "SwitcherStep",
    // "SwitcherStepVertical", "TriggerTiming" (lift | midswipe),
    // "TriggerDistance", and gesture keys.
    void load(const QMap<QString, QString> &overrides);

    Action action(GestureType type, int fingers, Direction direction, States states = {}) const;
    // True when every direction of this gesture is handled by us, so we can
    // take the whole gesture from its first event.
    bool handlesAllDirections(GestureType type, int fingers, States states = {}) const;
    // True when at least one direction is handled by us.
    bool handlesAnyDirection(GestureType type, int fingers, States states = {}) const;

    qreal swipeThreshold() const { return m_swipeThreshold; }
    qreal pinchThreshold() const { return m_pinchThreshold; }
    qreal switcherStep() const { return m_switcherStep; }
    qreal switcherStepVertical() const { return m_switcherStepVertical; }
    ActionTrigger::Timing triggerTiming() const { return m_triggerTiming; }
    qreal triggerDistance() const { return m_triggerDistance; }

private:
    QHash<QString, Action> m_bindings;
    qreal m_swipeThreshold = 30.0;
    qreal m_pinchThreshold = 0.2;
    qreal m_switcherStep = 100.0;
    // Touchpads are wider than tall, so up/down needs less travel by default.
    qreal m_switcherStepVertical = 60.0;
    ActionTrigger::Timing m_triggerTiming = ActionTrigger::Timing::OnLift;
    qreal m_triggerDistance = 80.0;
};

} // namespace WinLike
