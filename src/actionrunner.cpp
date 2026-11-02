/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "actionrunner.h"
#include "logging.h"

#include <input.h>
#include <keyboard_input.h>
#include <xkb.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QKeySequence>
#include <QProcess>
#include <QRegularExpression>

#include <linux/input-event-codes.h>

#include <chrono>

namespace WinLike
{

namespace
{

// kglobalaccel exports each component under a path built from its name with
// anything that is not [A-Za-z0-9_] replaced by '_'.
QString componentPath(const QString &component)
{
    static const QRegularExpression invalid(QStringLiteral("[^A-Za-z0-9_]"));
    QString escaped = component;
    escaped.replace(invalid, QStringLiteral("_"));
    return QStringLiteral("/component/") + escaped;
}

void invokeShortcut(const QString &component, const QString &name)
{
    // kglobalaccel lives inside the kwin_wayland process, so a blocking call
    // here would deadlock KWin. send() is fire-and-forget.
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kglobalaccel"),
                                                          componentPath(component),
                                                          QStringLiteral("org.kde.kglobalaccel.Component"),
                                                          QStringLiteral("invokeShortcut"));
    message << name;
    if (!QDBusConnection::sessionBus().send(message)) {
        qCWarning(WINLIKE) << "failed to invoke shortcut" << component << name;
    }
}

// Presses and releases a key combination ("Ctrl+Shift+T", "Left") as if typed,
// through KWin's own input path. Everything pressed here is released before
// returning, so no fake key can stay held.
void pressKey(const QString &text)
{
    const QKeySequence sequence = QKeySequence::fromString(text, QKeySequence::PortableText);
    if (sequence.isEmpty()) {
        qCWarning(WINLIKE) << "not a key:" << text;
        return;
    }
    const QKeyCombination combination = sequence[0];
    KWin::Xkb *xkb = KWin::input()->keyboard()->xkb();
    std::optional<KWin::Xkb::KeyCode> code;
    for (xkb_keysym_t sym : KWin::Xkb::keysymsFromQtKey(QKeyCombination(combination.key()))) {
        if ((code = xkb->keycodeFromKeysym(sym))) {
            break;
        }
    }
    if (!code) {
        qCWarning(WINLIKE) << "no key for" << text << "on the current keyboard layout";
        return;
    }

    QList<uint32_t> modifiers;
    const Qt::KeyboardModifiers mods = combination.keyboardModifiers();
    if (mods & Qt::ControlModifier) {
        modifiers << KEY_LEFTCTRL;
    }
    // A symbol on a shifted level of the layout (e.g. '!') needs Shift too.
    if ((mods & Qt::ShiftModifier) || code->level > 0) {
        modifiers << KEY_LEFTSHIFT;
    }
    if (mods & Qt::AltModifier) {
        modifiers << KEY_LEFTALT;
    }
    if (mods & Qt::MetaModifier) {
        modifiers << KEY_LEFTMETA;
    }

    const auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
    KWin::KeyboardInputRedirection *keyboard = KWin::input()->keyboard();
    // xkb keycodes are evdev codes + 8.
    const uint32_t key = code->keyCode - 8;
    for (uint32_t m : modifiers) {
        keyboard->processKey(m, KWin::KeyboardKeyState::Pressed, time);
    }
    keyboard->processKey(key, KWin::KeyboardKeyState::Pressed, time);
    keyboard->processKey(key, KWin::KeyboardKeyState::Released, time);
    for (auto it = modifiers.crbegin(); it != modifiers.crend(); ++it) {
        keyboard->processKey(*it, KWin::KeyboardKeyState::Released, time);
    }
}

} // namespace

void runAction(const Action &action)
{
    qCDebug(WINLIKE) << "running" << action.toString();
    switch (action.kind) {
    case Action::Kind::Shortcut:
        invokeShortcut(action.component, action.name);
        break;
    case Action::Kind::Command:
        if (!QProcess::startDetached(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), action.name})) {
            qCWarning(WINLIKE) << "failed to start" << action.name;
        }
        break;
    case Action::Kind::Key:
        pressKey(action.name);
        break;
    case Action::Kind::Switcher: // driven live by GestureFilter, nothing to do at the end
    case Action::Kind::Plasma: // handed to KWin by GestureFilter
    case Action::Kind::Block:
    case Action::Kind::Passthrough:
        break;
    }
}

} // namespace WinLike
