/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "gestureconfig.h"

#include <plugin.h>

#include <KConfigWatcher>
#include <KSharedConfig>

#include <memory>

namespace WinLike
{

class GestureFilter;

class WinLikeGestures : public KWin::Plugin
{
    Q_OBJECT

public:
    WinLikeGestures();
    ~WinLikeGestures() override;

private:
    void reload();

    GestureConfig m_config;
    KSharedConfig::Ptr m_sharedConfig;
    KConfigWatcher::Ptr m_watcher;
    std::unique_ptr<GestureFilter> m_filter;
};

} // namespace WinLike
