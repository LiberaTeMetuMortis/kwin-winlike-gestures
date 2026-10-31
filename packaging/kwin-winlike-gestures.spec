#
# spec file for package kwin-winlike-gestures
#
# SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
# SPDX-License-Identifier: GPL-2.0-or-later
#

Name:           kwin-winlike-gestures
# Set by OBS's set_version service from the newest v* git tag.
Version:        0
Release:        0
Summary:        Configurable Windows-style touchpad gestures for KWin
License:        GPL-2.0-or-later
URL:            https://github.com/LiberaTeMetuMortis/kwin-winlike-gestures
# Made by OBS's obs_scm + tar services from the git checkout.
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  cmake >= 3.22
BuildRequires:  gcc-c++
BuildRequires:  kf6-extra-cmake-modules
BuildRequires:  kwin6-devel
BuildRequires:  qt6-base-private-devel
BuildRequires:  cmake(KF6Config)
BuildRequires:  cmake(KF6CoreAddons)
BuildRequires:  cmake(KF6I18n)
BuildRequires:  cmake(KF6KCMUtils)
BuildRequires:  cmake(KF6WindowSystem)
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6DBus)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Quick)
BuildRequires:  cmake(Qt6Test)
BuildRequires:  cmake(Qt6Widgets)
# KWin's CMake package needs these but kwin6-devel does not pull them in.
BuildRequires:  pkgconfig(epoxy)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(vulkan)
BuildRequires:  pkgconfig(wayland-server)
# Binary KWin plugins only load into the exact KWin version they were built
# against; OBS rebuilds this package whenever kwin6 changes. Pin the library
# the plugin links (always in the build root via kwin6-devel); kwin6 itself
# is not installed there, so %%requires_eq can't read its version.
%requires_eq    libkwin6
Requires:       kwin6
Requires:       kf6-kcmutils
Requires:       kf6-kirigami

%description
A KWin plugin for Plasma 6 on Wayland that adds three- and four-finger
touchpad gestures with Windows-style defaults: Overview, Show Desktop, a live
Alt+Tab window switcher and virtual desktop switching. Every gesture can be
changed in System Settings > Mouse & Touchpad > Touchpad Gestures.

The plugin is off by default. Turn it on from that settings page.

%prep
%autosetup -p1

%build
%cmake -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install

%check
%ctest

%files
%license LICENSE
%doc README.md
%dir %{_libdir}/qt6/plugins/kwin
%dir %{_libdir}/qt6/plugins/kwin/plugins
%{_libdir}/qt6/plugins/kwin/plugins/winlikegestures.so
%dir %{_libdir}/qt6/plugins/plasma
%dir %{_libdir}/qt6/plugins/plasma/kcms
%dir %{_libdir}/qt6/plugins/plasma/kcms/systemsettings
%{_libdir}/qt6/plugins/plasma/kcms/systemsettings/kcm_winlikegestures.so
%{_datadir}/applications/kcm_winlikegestures.desktop
%dir %{_datadir}/qlogging-categories6
%{_datadir}/qlogging-categories6/kwin_winlike_gestures.categories

%changelog
