/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "winlikeplugin.h"
#include "gesturefilter.h"
#include "logging.h"

#include <input.h>
#include <workspace.h>

#include <KConfigGroup>

Q_LOGGING_CATEGORY(WINLIKE, "kwin_winlike_gestures", QtInfoMsg)

namespace WinLike
{

namespace
{
const QString s_configName = QStringLiteral("kwinwinlikegesturesrc");
const QString s_group = QStringLiteral("Gestures");
}

WinLikeGestures::WinLikeGestures()
    : m_sharedConfig(KSharedConfig::openConfig(s_configName, KConfig::SimpleConfig))
    , m_filter(std::make_unique<GestureFilter>(&m_config))
{
    // The KDE way to follow config changes: whoever writes the file with
    // KConfig::Notify (a KCM, `kwriteconfig6 --notify`) broadcasts it over
    // D-Bus. The watcher reparses the config before emitting, and stays idle
    // otherwise.
    m_watcher = KConfigWatcher::create(m_sharedConfig);
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group) {
        if (group.name() == s_group) {
            reload();
        }
    });

    // Manual refresh after hand-editing the file:
    //   qdbus6 org.kde.KWin /KWin reconfigure
    connect(KWin::workspace(), &KWin::Workspace::configChanged, this, [this] {
        m_sharedConfig->reparseConfiguration();
        reload();
    });

    reload();
    KWin::input()->installInputEventFilter(m_filter.get());
    qCInfo(WINLIKE) << "loaded";
}

// GestureFilter's destructor uninstalls it from KWin's input chain.
WinLikeGestures::~WinLikeGestures()
{
    qCInfo(WINLIKE) << "unloaded";
}

void WinLikeGestures::reload()
{
    m_config.load(m_sharedConfig->group(s_group).entryMap());
    qCDebug(WINLIKE) << "config loaded";
}

} // namespace WinLike
