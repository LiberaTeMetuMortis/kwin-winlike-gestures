/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "actionrunner.h"
#include "logging.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QProcess>
#include <QRegularExpression>

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
        break;
    case Action::Kind::Switcher: // driven live by GestureFilter, nothing to do at the end
    case Action::Kind::Plasma: // handed to KWin by GestureFilter
    case Action::Kind::Block:
    case Action::Kind::Passthrough:
        break;
    }
}

} // namespace WinLike
