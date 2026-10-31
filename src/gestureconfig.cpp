/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "gestureconfig.h"

#include <initializer_list>

namespace WinLike
{

namespace
{

const QString s_shortcutPrefix = QStringLiteral("shortcut:");
const QString s_commandPrefix = QStringLiteral("command:");

std::initializer_list<Direction> directionsOf(GestureType type)
{
    static const auto swipe = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    static const auto pinch = {Direction::In, Direction::Out};
    return type == GestureType::Pinch ? pinch : swipe;
}

qreal positiveOr(const QString &value, qreal fallback)
{
    bool ok = false;
    const qreal v = value.toDouble(&ok);
    return ok && v > 0 ? v : fallback;
}

} // namespace

Action Action::parse(const QString &spec)
{
    const QString s = spec.trimmed();
    if (s == QLatin1String("block")) {
        return {Kind::Block, {}, {}};
    }
    if (s == QLatin1String("switcher")) {
        return {Kind::Switcher, {}, {}};
    }
    if (s.startsWith(s_shortcutPrefix)) {
        const QString rest = s.mid(s_shortcutPrefix.size());
        const int slash = rest.indexOf(QLatin1Char('/'));
        if (slash > 0 && slash < rest.size() - 1) {
            return {Kind::Shortcut, rest.left(slash), rest.mid(slash + 1)};
        }
        return {};
    }
    if (s.startsWith(s_commandPrefix)) {
        const QString command = s.mid(s_commandPrefix.size()).trimmed();
        if (!command.isEmpty()) {
            return {Kind::Command, {}, command};
        }
    }
    return {};
}

QString Action::toString() const
{
    switch (kind) {
    case Kind::Block:
        return QStringLiteral("block");
    case Kind::Shortcut:
        return s_shortcutPrefix + component + QLatin1Char('/') + name;
    case Kind::Command:
        return s_commandPrefix + name;
    case Kind::Switcher:
        return QStringLiteral("switcher");
    case Kind::Passthrough:
        break;
    }
    return QStringLiteral("passthrough");
}

GestureConfig::GestureConfig()
{
    load({});
}

QString GestureConfig::key(GestureType type, int fingers, Direction direction)
{
    const QString prefix = type == GestureType::Pinch ? QStringLiteral("Pinch") : QStringLiteral("Swipe");
    return prefix + QString::number(fingers) + directionName(direction);
}

QMap<QString, QString> GestureConfig::defaults()
{
    return {
        {QStringLiteral("Swipe3Up"), QStringLiteral("shortcut:kwin/Overview")},
        {QStringLiteral("Swipe3Down"), QStringLiteral("shortcut:kwin/Show Desktop")},
        {QStringLiteral("Swipe3Left"), QStringLiteral("switcher")},
        {QStringLiteral("Swipe3Right"), QStringLiteral("switcher")},
        {QStringLiteral("Swipe4Up"), QStringLiteral("shortcut:kwin/Overview")},
        {QStringLiteral("Swipe4Down"), QStringLiteral("shortcut:kwin/Show Desktop")},
        // Content follows the fingers: swiping left brings in the next desktop.
        // Next/Previous rather than Left/Right so it works with any desktop grid.
        {QStringLiteral("Swipe4Left"), QStringLiteral("shortcut:kwin/Switch to Next Desktop")},
        {QStringLiteral("Swipe4Right"), QStringLiteral("shortcut:kwin/Switch to Previous Desktop")},
        // Up and down undo each other, like on Windows. Both shortcuts toggle.
        {QStringLiteral("Swipe3Down.Overview"), QStringLiteral("shortcut:kwin/Overview")},
        {QStringLiteral("Swipe4Down.Overview"), QStringLiteral("shortcut:kwin/Overview")},
        {QStringLiteral("Swipe3Up.Desktop"), QStringLiteral("shortcut:kwin/Show Desktop")},
        {QStringLiteral("Swipe4Up.Desktop"), QStringLiteral("shortcut:kwin/Show Desktop")},
    };
}

void GestureConfig::load(const QMap<QString, QString> &overrides)
{
    QMap<QString, QString> merged = defaults();
    for (auto it = overrides.cbegin(); it != overrides.cend(); ++it) {
        merged.insert(it.key(), it.value());
    }

    m_swipeThreshold = positiveOr(merged.take(QStringLiteral("Threshold")), 30.0);
    m_pinchThreshold = positiveOr(merged.take(QStringLiteral("PinchThreshold")), 0.2);
    m_switcherStep = positiveOr(merged.take(QStringLiteral("SwitcherStep")), 100.0);
    m_switcherStepVertical = positiveOr(merged.take(QStringLiteral("SwitcherStepVertical")), 60.0);
    m_triggerTiming = merged.take(QStringLiteral("TriggerTiming")).trimmed() == QLatin1String("midswipe") ? ActionTrigger::Timing::MidSwipe
                                                                                                         : ActionTrigger::Timing::OnLift;
    m_triggerDistance = positiveOr(merged.take(QStringLiteral("TriggerDistance")), 80.0);
    m_reverseUndo = merged.take(QStringLiteral("ReverseUndo")).trimmed() == QLatin1String("true");

    m_bindings.clear();
    for (auto it = merged.cbegin(); it != merged.cend(); ++it) {
        // "inherit" cancels a default, e.g. a built-in state binding.
        if (it.value().trimmed() != QLatin1String("inherit")) {
            m_bindings.insert(it.key(), Action::parse(it.value()));
        }
    }
}

Action GestureConfig::action(GestureType type, int fingers, Direction direction, States states) const
{
    const QString base = key(type, fingers, direction);
    // Overview is checked first: it is drawn on top of a shown desktop.
    static const std::pair<State, QString> suffixes[] = {
        {State::OverviewOpen, QStringLiteral(".Overview")},
        {State::DesktopShown, QStringLiteral(".Desktop")},
    };
    for (const auto &[state, suffix] : suffixes) {
        if (states.testFlag(state)) {
            const auto it = m_bindings.constFind(base + suffix);
            if (it != m_bindings.cend()) {
                return *it;
            }
        }
    }
    return m_bindings.value(base);
}

bool GestureConfig::handlesAllDirections(GestureType type, int fingers, States states) const
{
    for (Direction d : directionsOf(type)) {
        if (!action(type, fingers, d, states).handled()) {
            return false;
        }
    }
    return true;
}

bool GestureConfig::handlesAnyDirection(GestureType type, int fingers, States states) const
{
    for (Direction d : directionsOf(type)) {
        if (action(type, fingers, d, states).handled()) {
            return true;
        }
    }
    return false;
}

} // namespace WinLike
