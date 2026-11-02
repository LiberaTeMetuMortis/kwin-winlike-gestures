# SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.22)
# The version lives only in git tags (vX.Y.Z); OBS reads it from there.
project(kwin-winlike-gestures LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Install into the system Qt plugin dir so kwin_wayland can find us.
set(KDE_INSTALL_USE_QT_SYS_PATHS ON)
include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui DBus Widgets Quick Test)
find_package(KWin REQUIRED)
# KWin's xkb.h includes xkbcommon, which openSUSE keeps outside /usr/include.
find_package(PkgConfig REQUIRED)
pkg_check_modules(XKBCommon REQUIRED IMPORTED_TARGET xkbcommon)

# Pure logic, no KWin dependency: unit-testable.
add_library(winlike_core STATIC src/recognizer.cpp src/gestureconfig.cpp src/keypress.cpp)
target_include_directories(winlike_core PUBLIC src)
target_link_libraries(winlike_core PUBLIC Qt6::Core)
set_target_properties(winlike_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(winlikegestures MODULE
    src/main.cpp
    src/winlikeplugin.cpp
    src/gesturefilter.cpp
    src/actionrunner.cpp
)
target_link_libraries(winlikegestures PRIVATE winlike_core KWin::kwin KF6::ConfigCore Qt6::DBus PkgConfig::XKBCommon)

# KWin keeps the plugin it loaded mapped in memory, so the installed file must
# never be overwritten in place (that can crash KWin). When the build differs,
# remove the old file first so the new one gets a fresh inode; when it's
# identical, leave it alone.
set(WINLIKE_PLUGIN_DIR "${KDE_INSTALL_FULL_PLUGINDIR}/kwin/plugins")
install(CODE "
    set(_dst \"\$ENV{DESTDIR}${WINLIKE_PLUGIN_DIR}/winlikegestures.so\")
    execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E compare_files \"$<TARGET_FILE:winlikegestures>\" \"\${_dst}\"
                    RESULT_VARIABLE _differs OUTPUT_QUIET ERROR_QUIET)
    if(_differs)
        file(REMOVE \"\${_dst}\")
        file(INSTALL DESTINATION \"${WINLIKE_PLUGIN_DIR}\" TYPE MODULE FILES \"$<TARGET_FILE:winlikegestures>\")
    else()
        message(STATUS \"Unchanged: \${_dst}\")
    endif()
")
# Every installed file for this distro's layout, plugin first. install.sh and
# uninstall.sh read it, so they never hardcode paths.
file(WRITE ${CMAKE_BINARY_DIR}/installed-files.txt
"${WINLIKE_PLUGIN_DIR}/winlikegestures.so
${KDE_INSTALL_FULL_PLUGINDIR}/plasma/kcms/systemsettings/kcm_winlikegestures.so
${KDE_INSTALL_FULL_APPDIR}/kcm_winlikegestures.desktop
${KDE_INSTALL_FULL_LOGGINGCATEGORIESDIR}/kwin_winlike_gestures.categories
")

# Lets KDebugSettings switch the plugin's debug logging on and off.
install(FILES kwin_winlike_gestures.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

# System Settings page (Input & Output > Mouse & Touchpad > Touchpad Gestures).
option(BUILD_KCM "Build the System Settings page" ON)
if(BUILD_KCM)
    add_subdirectory(kcm)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(test_recognizer tests/test_recognizer.cpp)
    target_link_libraries(test_recognizer PRIVATE winlike_core Qt6::Test)
    add_test(NAME recognizer COMMAND test_recognizer)
    add_executable(test_gestureconfig tests/test_gestureconfig.cpp)
    target_link_libraries(test_gestureconfig PRIVATE winlike_core Qt6::Test)
    add_test(NAME gestureconfig COMMAND test_gestureconfig)
    add_executable(test_keypress tests/test_keypress.cpp)
    target_link_libraries(test_keypress PRIVATE winlike_core Qt6::Test)
    add_test(NAME keypress COMMAND test_keypress)
endif()
