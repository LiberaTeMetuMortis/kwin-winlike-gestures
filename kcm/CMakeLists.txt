# SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(KF6 6.0 REQUIRED COMPONENTS KCMUtils I18n)

# Picks up ui/*.qml and installs into plasma/kcms/systemsettings.
kcmutils_add_qml_kcm(kcm_winlikegestures SOURCES kcm_winlikegestures.cpp)
target_compile_definitions(kcm_winlikegestures PRIVATE TRANSLATION_DOMAIN="kcm_winlikegestures")
target_link_libraries(kcm_winlikegestures PRIVATE
    winlike_core
    KF6::KCMUtilsQuick
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    Qt6::DBus
)
