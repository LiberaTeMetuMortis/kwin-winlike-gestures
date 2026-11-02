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
    GridOpen = 1 << 2, // suffix ".Grid": the desktop grid (Overview's grid mode)
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
        Plasma, // hand the swipe to one of KWin's own gestures, which follow the fingers
    };

    Kind kind = Kind::Passthrough;
    QString component;
    QString name; // shortcut name, or the command line
    // Plasma: the KWin touchpad gesture the swipe is handed to.
    int targetFingers = 0;
    Direction targetDirection = Direction::None;

    static Action parse(const QString &spec);
    QString toString() const;
    bool handled() const { return kind != Kind::Passthrough; }
    // Whether running this can undo a swipe (swipe back to undo): only
    // actions that do something. The switcher, block and passthrough can't.
    bool canUndo() const { return kind == Kind::Shortcut || kind == Kind::Command; }
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
    // Fallbacks for the tuning keys, as config-file strings.
    static QMap<QString, QString> tuningDefaults();
    // The entries of a config file that differ from the built-in defaults
    // (bindings and tuning). A state key equal to its normal gesture is kept:
    // it was set on purpose and must not start following the normal gesture.
    static QMap<QString, QString> overrides(const QMap<QString, QString> &entries);

    // Applies overrides. Keys: "Threshold", "PinchThreshold", "SwitcherStep",
    // "SwitcherStepVertical", "TriggerTiming" (lift | midswipe),
    // "TriggerDistance", "ReverseUndo" (true | false), and gesture keys.
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
    // Swipe back to undo (only with mid-swipe timing).
    bool reverseUndo() const { return m_reverseUndo; }

private:
    QHash<QString, Action> m_bindings;
    qreal m_swipeThreshold = 30.0;
    qreal m_pinchThreshold = 0.2;
    qreal m_switcherStep = 100.0;
    // Touchpads are wider than tall, so up/down needs less travel by default.
    qreal m_switcherStepVertical = 60.0;
    ActionTrigger::Timing m_triggerTiming = ActionTrigger::Timing::OnLift;
    qreal m_triggerDistance = 80.0;
    bool m_reverseUndo = false;
};

} // namespace WinLike
