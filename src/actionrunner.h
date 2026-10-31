/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "gestureconfig.h"

namespace WinLike
{

// Executes an Action without ever blocking KWin's event loop.
void runAction(const Action &action);

} // namespace WinLike
