# kwin-winlike-gestures

Windows-style touchpad gestures for KDE Plasma 6 on Wayland, configurable per gesture.

It comes in two parts:

* **A KWin plugin** (a compiled C++ plugin, not a KWin script). It runs inside the
  compositor, sees touchpad gestures *before* KWin's own gesture bindings do, and maps
  them to actions you choose. Because it takes gestures over completely, Plasma's
  built-in gestures can't fire alongside it.
* **A System Settings page**, *Mouse & Touchpad → Touchpad Gestures*, where you turn the
  plugin on or off and pick what each gesture does. No config file editing needed.

Plasma ships a few built-in touchpad gestures, but you can't change them. The defaults
here copy a Windows precision touchpad.

Built and tested against **KWin / Plasma 6.8.1** on openSUSE Tumbleweed.

**openSUSE Tumbleweed:** install the package from OBS (rebuilt automatically for every
KWin update; see [Shipping](#shipping)), then turn it on in **System Settings → Mouse &
Touchpad → Touchpad Gestures**. **Arch, Fedora, Debian/Ubuntu and others:**
[install the dependencies](#dependencies), then run `./install.sh`.

---

## Default gestures

| Gesture | Action |
|---|---|
| 3-finger swipe up | Overview (like Windows Task View). **If the desktop is shown:** bring the windows back |
| 3-finger swipe down | Show Desktop. **If Overview is open:** close Overview |
| 3-finger swipe left / right | **Alt+Tab switcher.** Opens and stays open while your fingers are down; keep sliding left/right/up/down to move the selection (like the arrow keys); lift to switch. A quick flick jumps to the previous window. |
| 4-finger swipe up | Same as 3-finger up (including the desktop-shown rule) |
| 4-finger swipe down | Same as 3-finger down (including the Overview rule) |
| 4-finger swipe left / right | Next / previous virtual desktop (content follows your fingers) |
| 2-finger scroll, pinch-zoom, 2-finger tap | Untouched: handled by libinput and apps, already like Windows |

Every binding can be changed in **System Settings → Mouse & Touchpad → Touchpad
Gestures** (see [Settings page](#settings-page)) or in the config file (see
[Configuration](#configuration)).

---

## How it works

### Why a KWin plugin

On Wayland the compositor (KWin) owns the input devices. Outside tools like
`libinput-gestures`, `touchegg` or `fusuma` either can't read touchpad gestures at all,
or can't stop KWin's built-in gestures from firing too. The only reliable place to
handle gestures is **inside KWin's input pipeline**. Binary KWin plugins
(`KWin::Plugin`) are loaded into the `kwin_wayland` process and can install an
`InputEventFilter` there.

### The input pipeline

KWin runs every input event through a chain of `InputEventFilter`s, ordered by weight
(`InputFilterOrder` in `kwin/input.h`). Each filter returns `true` to consume the event
(later filters never see it) or `false` to pass it on:

```
libinput ─► … ─► LockScreen ─► ScreenEdge ─► DragAndDrop ─► [WindowSelector: OUR FILTER]
                                                             │
           ◄── consumed if the gesture is ours ──────────────┘
                                                             ▼
                          TabBox ─► GlobalShortcut (KWin's built-in gestures) ─► … ─► Forward (apps)
```

* We come **after LockScreen**, so nothing fires on the lock screen.
* We come **before GlobalShortcut**, where KWin's built-in touchpad gestures live, so
  we can take a gesture away from KWin.
* We come **before TabBox**, so we keep receiving swipe updates while the Alt+Tab
  switcher we opened is on screen.

### Taking ownership of a gesture

When libinput reports a swipe start, we only know the finger count, not the direction.
`GestureFilter` puts each gesture in one of three modes:

| Mode | When | Behaviour |
|---|---|---|
| **Owned** | every direction for this finger count is bound to one of our actions | Consume from the first event. KWin and apps never see the gesture. |
| **Watching** | only some directions are ours | Pass events through until the direction locks. If it's ours, send a `swipeGestureCancelled` down the rest of the chain (via `input()->processFilters`) and switch to Owned. Otherwise switch to Ignored. |
| **Ignored** | nothing bound for this finger count | Pass everything through. |

With the defaults, both 3- and 4-finger swipes are fully bound, so they're always
**Owned**. Watching mode only applies if you set some directions to `passthrough`.
Cancelling KWin's gesture partway through isn't fully reliable (KWin's animation may
already have started), so binding all four directions of a finger count gives the
cleanest result.

### Direction recognition

`SwipeRecognizer` adds up libinput deltas (+x right, +y down). Once the total movement
passes `Threshold` (default 30 units), it locks onto the dominant axis. The lock never
flips during a gesture. `PinchRecognizer` does the same for pinch scale (In below
`1 - PinchThreshold`, Out above `1 + PinchThreshold`).

Discrete actions (shortcuts, commands) run when the fingers **lift** by default, and only
if a direction locked. With `TriggerTiming=midswipe` they run **mid-swipe** instead, as
soon as the fingers have travelled `TriggerDistance` (default 80 units) along the locked
direction, so Overview opens while you're still swiping. A swipe that never gets that
far does nothing. Pinches have no travel distance, so in mid-swipe mode they run as soon
as the pinch direction locks. `ActionTrigger` makes sure an action fires at most once per
gesture. A cancelled gesture does nothing. The window switcher follows the fingers live
in either mode.

### Running actions

* **`shortcut:`** sends `invokeShortcut` to `org.kde.kglobalaccel`. On Plasma 6, the
  kglobalaccel service **runs inside `kwin_wayland` itself**, so a blocking D-Bus call
  from the plugin would make KWin wait on itself forever. The plugin only uses
  fire-and-forget `QDBusConnection::send()`.
* **`command:`** runs `/bin/sh -c <command>` detached with `QProcess::startDetached`.

### The live Alt+Tab switcher

KWin's TabBox (the Alt+Tab switcher) stays open as long as Alt is held. The plugin
recreates that with key events injected through KWin's own keyboard pipeline
(`input()->keyboard()->processKey`):

1. When the direction locks (3 fingers, horizontal): **Alt down, Tab tap**. The switcher
   opens with the next window selected.
2. While fingers move: `SwitcherNavigator` turns travel into steps, one per
   `SwitcherStep` units sideways (default 100) and `SwitcherStepVertical` up/down
   (default 60, since touchpads are wider than tall). Horizontal steps tap **Left/Right**,
   vertical steps tap **Up/Down**, exactly the arrow keys TabBox uses to move the
   selection (Up/Down move between rows in grid layouts). When one axis steps, the other
   axis's partial travel is dropped, so the drift of a sideways slide never adds up to
   an accidental up/down step.
3. Fingers lift: **Alt up**, and TabBox switches to the selected window.
4. Gesture cancelled: **Esc tap, Alt up**. The switcher closes without switching.

The filter's destructor releases Alt if the plugin is unloaded mid-gesture, so Alt
can't get stuck down.

### Config reload

The plugin follows KDE's standard pattern instead of polling or watching files:

* **Automatic:** a `KConfigWatcher` listens for change notifications. Anything that
  writes the config with `KConfig::Notify` (a System Settings page, or
  `kwriteconfig6 --notify`) broadcasts the change over D-Bus. The watcher reparses the
  file and the plugin reloads. Nothing else wakes the plugin up.
* **Manual:** after editing the file by hand, run KWin's own "reload settings" command:

  ```sh
  qdbus6 org.kde.KWin /KWin reconfigure
  ```

  This emits `Workspace::configChanged`, and the plugin rereads its file.

The new bindings apply to the next gesture.

### State-dependent bindings

When a gesture starts, the plugin takes a snapshot of two desktop states:

* **Overview open:** `effects->isEffectActive("overview")`
* **Desktop shown:** `workspace()->showingDesktop()`

A binding with a state suffix (`Swipe3Down.Overview`, `Swipe3Up.Desktop`) wins over the
plain one while that state holds. If both states hold, `.Overview` is checked first
(Overview is drawn on top). That snapshot is used for the whole gesture, so the state
changing halfway through (e.g. Overview opening) doesn't switch actions mid-swipe.

---

## Code architecture

```
kde-gestures/
├── CMakeLists.txt            build: core lib, plugin module, tests, safe plugin install,
│                             build/installed-files.txt (paths for the scripts)
├── install.sh / uninstall.sh build+test+install / remove, run as your normal user
├── packaging/                OBS spec file
├── kwin_winlike_gestures.categories  debug-log switch for KDebugSettings
├── src/
│   ├── main.cpp              WinLikeGesturesFactory: the KWin::PluginFactory entry point
│   ├── metadata.json         plugin metadata (EnabledByDefault: false)
│   ├── winlikeplugin.h/.cpp  WinLikeGestures (KWin::Plugin): owns config + filter, installs
│   │                         the filter, reloads config via KConfigWatcher / KWin reconfigure
│   ├── gesturefilter.h/.cpp  GestureFilter (KWin::InputEventFilter): Owned/Watching/Ignored
│   │                         logic, desktop-state snapshot, cancel injection, live switcher
│   ├── actionrunner.h/.cpp   runAction(): non-blocking kglobalaccel calls and commands
│   ├── gestureconfig.h/.cpp  Action parsing, built-in defaults, overrides, state-specific
│   │                         bindings, coverage queries
│   ├── recognizer.h/.cpp     SwipeRecognizer, PinchRecognizer, SwitcherStepper,
│   │                         SwitcherNavigator, ActionTrigger
│   └── logging.h             WINLIKE logging category (kwin_winlike_gestures)
├── kcm/                      System Settings page (separate .so, runs in systemsettings)
│   ├── CMakeLists.txt        kcmutils_add_qml_kcm; optional via -DBUILD_KCM=OFF
│   ├── kcm_winlikegestures.json  page metadata: name, icon, category "pointing-devices"
│   ├── kcm_winlikegestures.h/.cpp  KQuickConfigModule: load/save/defaults, enable switch,
│   │                         KWin shortcut list; reuses GestureConfig::defaults()
│   └── ui/
│       ├── main.qml          the form: sections per finger count, states, pinch, sliders
│       └── GestureRow.qml    one binding: action dropdown + shortcut picker / command field
└── tests/
    ├── test_recognizer.cpp   direction lock, threshold, progress, reset, pinch, stepping
    └── test_gestureconfig.cpp parsing, round-trip, defaults, overrides, states, bad values
```

The code is split in two layers on purpose:

* **`winlike_core`** (static lib: `recognizer`, `gestureconfig`) depends only on QtCore.
  All the decision logic lives here and is unit-tested without KWin.
* **`kcm_winlikegestures.so`** (settings page) links `winlike_core` too, so the page
  always shows the same defaults the plugin uses.
* **`winlikegestures.so`** (plugin module) is the thin layer that touches KWin's
  **internal, unstable** API: the input filter, key injection and plugin glue. Keeping it
  small keeps the work after each Plasma update small.

---

## Building

### Dependencies

You need **Plasma 6 on Wayland** and the development files for the KWin you're running.
These packages only add headers and build files; nothing on the running desktop changes.

**openSUSE Tumbleweed** (tested):

```sh
sudo zypper in kwin6-devel kf6-extra-cmake-modules kf6-kconfig-devel \
    kf6-kglobalaccel-devel kf6-kcoreaddons-devel kf6-kwindowsystem-devel \
    qt6-base-private-devel qt6-dbus-devel qt6-declarative-devel libinput-devel \
    kf6-kcmutils-devel kf6-ki18n-devel cmake gcc-c++
```

**Arch Linux / Manjaro / EndeavourOS** (Arch doesn't split off dev packages; the `kwin`
package already contains its headers):

```sh
sudo pacman -S --needed base-devel cmake extra-cmake-modules kwin \
    kcmutils ki18n kconfig kcoreaddons kwindowsystem \
    qt6-base qt6-declarative vulkan-headers
```

**Fedora** (KDE Plasma spin, Fedora 40 or newer):

```sh
sudo dnf install cmake gcc-c++ extra-cmake-modules kwin-devel \
    kf6-kcmutils-devel kf6-ki18n-devel kf6-kconfig-devel kf6-kcoreaddons-devel \
    kf6-kwindowsystem-devel qt6-qtbase-devel qt6-qtbase-private-devel \
    qt6-qtdeclarative-devel libepoxy-devel libdrm-devel vulkan-loader-devel \
    vulkan-headers wayland-devel
```

**Debian 13 (trixie) or newer / Kubuntu 24.10 or newer / KDE neon:**

```sh
sudo apt install cmake g++ extra-cmake-modules kwin-dev \
    libkf6kcmutils-dev libkf6i18n-dev libkf6config-dev libkf6coreaddons-dev \
    libkf6windowsystem-dev qt6-base-dev qt6-base-private-dev qt6-declarative-dev \
    libepoxy-dev libdrm-dev libvulkan-dev libwayland-dev
```

Kubuntu 24.04 LTS and Debian 12 ship Plasma 5, which this plugin doesn't support.

Only the openSUSE list has been tested. The others come from each distro's KWin
packaging (Debian's `kwin-dev`, Fedora's `kwin-devel`, Arch's `kwin` file list); if one is
missing something, CMake names the missing package when you configure. Please open an
issue if your distro needs a correction.

Notes:

* Qt Quick (`qt6-declarative`) and the epoxy, DRM, Vulkan and Wayland headers are needed
  because **KWin's own CMake package** looks for them, even though this plugin doesn't use
  them directly. Some distros' KWin dev packages don't pull them in.
* **The KWin development files must match your installed KWin version exactly.** The plugin
  interface ID contains the version (`org.kde.kwin.PluginFactoryInterface6.8.1`), and KWin
  refuses plugins built for another version. Install them from the same repositories as
  your KWin and rebuild after every Plasma update.

### Build and test

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Outputs:

| File | Installed to |
|---|---|
| `build/bin/winlikegestures.so` (plugin) | `/usr/lib64/qt6/plugins/kwin/plugins/` |
| `build/bin/plasma/kcms/systemsettings/kcm_winlikegestures.so` (settings page) | `/usr/lib64/qt6/plugins/plasma/kcms/systemsettings/` |
| `build/kcm/kcm_winlikegestures.desktop` (launcher entry) | `/usr/share/applications/` |
| `kwin_winlike_gestures.categories` (debug-log switch for KDebugSettings) | `/usr/share/qlogging-categories6/` |

Build without the settings page with `-DBUILD_KCM=OFF`.

To try the settings page from the build folder, without installing:

```sh
QT_PLUGIN_PATH=$PWD/build/bin:$QT_PLUGIN_PATH kcmshell6 kcm_winlikegestures
```

---

## Installing and enabling

### Quick way

```sh
./install.sh
```

Run it as your normal user, **not** with `sudo`. It builds, runs the tests, and installs
the plugin, settings page and launcher entry, asking for your password only for the
copy step. It works out where your distro keeps Qt plugins, only replaces the KWin
plugin if it changed, and tells you whether you need to log out and back in. The manual
steps below do the same thing.

### 1. Install

KWin and System Settings only look for plugins in the system Qt plugin directory:

```sh
sudo cmake --install build
```

This is safe even while KWin has the old plugin loaded. KWin keeps that file mapped in
memory, so overwriting it in place could crash KWin. The install rule in `CMakeLists.txt`
removes the old file first when the build differs (the new one gets a fresh inode) and
leaves it untouched when it's identical.

### 2. Try it without enabling (temporary, until logout)

```sh
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadPlugin winlikegestures
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.UnloadPlugin winlikegestures
```

The plugin is not enabled by default (`EnabledByDefault: false`), so if a test build
crashes KWin, it won't be loaded again when KWin restarts.

### 3. Enable permanently (loads at every login)

Easiest: **System Settings → Mouse & Touchpad → Touchpad Gestures**, turn on
**Touchpad gestures**, press **Apply**. Or from a terminal:

```sh
kwriteconfig6 --file kwinrc --group Plugins --key winlikegesturesEnabled true
qdbus6 org.kde.KWin /KWin reconfigure      # or log out and back in
```

To disable:

```sh
kwriteconfig6 --file kwinrc --group Plugins --key winlikegesturesEnabled false
```

### Updating to a new build

Qt doesn't actually unload a plugin's code from memory, so `UnloadPlugin` +
`LoadPlugin` keeps running the **old** code. After installing a new build,
**log out and back in** (or restart KWin).

### After every Plasma update

KWin's internal API isn't stable, and plugins must be rebuilt for each KWin release:

```sh
sudo zypper in kwin6-devel            # update it to match the new KWin
rm -rf build && cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr && cmake --build build
ctest --test-dir build
sudo cmake --install build
```

(or just `./install.sh`, after updating `kwin6-devel`)

Then log out and back in. If KWin's headers changed, the build fails, and the small
`gesturefilter.cpp` / `winlikeplugin.cpp` layer is where to adapt. Until you rebuild, a
plugin built for the old version is simply refused, and gestures go back to Plasma's
defaults.

### Uninstall

```sh
./uninstall.sh            # keeps your gesture settings
./uninstall.sh --purge    # also deletes ~/.config/kwinwinlikegesturesrc
```

Run it as your normal user, **not** with `sudo`. It turns the plugin off in `kwinrc`,
unloads it from the running KWin, and removes every installed file. CMake writes the
list of installed paths for your distro to `build/installed-files.txt`, so nothing is
hardcoded. Plasma's own gestures come back immediately; log out and back in to fully
unload the old code. If you installed the OBS package instead, use
`sudo zypper rm kwin-winlike-gestures`.

### Recovery if KWin crashes at login

A crash in the plugin crashes KWin. If the session won't start, switch to a text console
(Ctrl+Alt+F3), log in, run the disable command above (or delete
`winlikegesturesEnabled=true` from `~/.config/kwinrc`), and log in again.

---

## Settings page

**System Settings → Mouse & Touchpad → Touchpad Gestures** (or `kcmshell6
kcm_winlikegestures`). It has:

* **Touchpad gestures on/off.** On Apply it writes `winlikegesturesEnabled` to `kwinrc`
  *and* loads or unloads the plugin in the running KWin, so it takes effect at once.
* A dropdown per gesture: 3- and 4-finger swipes, the *While Overview is open* and
  *While the desktop is shown* states, and 2/3-finger pinch. Choices: Overview, Show
  desktop, Window switcher, Next/Previous virtual desktop, Desktop grid, *Other KWin
  shortcut…* (picker listing every KWin shortcut), *Run command…*, Do nothing, Leave to
  Plasma or the app, and for state rows *Same as usual*.
* **Run actions:** *When fingers lift* (default) or *Mid-swipe, after a set distance*,
  with a slider for that distance.
* Sliders for swipe threshold, switcher sideways step, switcher up/down step and pinch
  threshold.
* **Defaults** restores the Windows-style bindings (it leaves the on/off switch alone).

On Apply, the page writes only values that differ from the built-in defaults, with
`KConfig::Notify`, so the running plugin reloads immediately. KConfig doesn't keep
comments, so hand-written comments in the config file are lost once you save from the
page.

## Configuration

File: `~/.config/kwinwinlikegesturesrc`. Only list what you want to change; anything
missing uses the built-in default. After editing by hand, apply it with:

```sh
qdbus6 org.kde.KWin /KWin reconfigure
```

or write values with `kwriteconfig6 --notify`, which applies them instantly:

```sh
kwriteconfig6 --notify --file kwinwinlikegesturesrc --group Gestures --key Swipe3Left switcher
```

```ini
[Gestures]
# Keys:   Swipe<fingers><Up|Down|Left|Right>, Pinch<fingers><In|Out>
Swipe3Up=shortcut:kwin/Overview
Swipe4Left=shortcut:kwin/Switch to Next Desktop
Swipe4Right=shortcut:kwin/Switch to Previous Desktop
Pinch3In=command:dolphin

# State-specific: wins over the plain key while that state holds
#   .Overview  Overview is open      .Desktop  Show Desktop is active
Swipe3Down.Overview=shortcut:kwin/Overview      # close Overview (the shortcut toggles)
Swipe3Up.Desktop=shortcut:kwin/Show Desktop     # bring the windows back

# Tuning
Threshold=30        # swipe distance before a direction locks
PinchThreshold=0.2  # scale change before a pinch locks
SwitcherStep=100            # sideways travel per switcher step (Left/Right)
SwitcherStepVertical=60     # up/down travel per switcher step (Up/Down)
TriggerTiming=lift          # lift = run actions when fingers lift; midswipe = while swiping
TriggerDistance=80          # midswipe only: travel before the action runs
```

| Value | Meaning |
|---|---|
| `shortcut:<component>/<name>` | Trigger a KDE global shortcut |
| `command:<shell command>` | Run a command |
| `switcher` | Live Alt+Tab switcher (meant for horizontal swipes) |
| `block` | Swallow the gesture and do nothing |
| `passthrough` | Leave the gesture to KWin / the focused app |
| `inherit` | Ignore this key. On a state key (`Swipe3Down.Overview`), it removes the built-in state rule so the normal binding applies |

Malformed values fall back to `passthrough`, so a typo never swallows your gestures.

To list the shortcut names available for `shortcut:` (`kwin` is the component):

```sh
qdbus6 org.kde.kglobalaccel /component/kwin org.kde.kglobalaccel.Component.shortcutNames
qdbus6 --literal org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel.allComponents   # other components
```

---

## Troubleshooting

Per-gesture logging is **off by default** (only `loaded`/`unloaded` are logged). To turn
it on, open **KDebugSettings** (`kdebugsettings`), find *Windows-like touchpad gestures
(KWin plugin)* and set it to *Full Debug*. Or add this to
`~/.config/QtProject/qtlogging.ini`:

```ini
[Rules]
kwin_winlike_gestures.debug=true
```

KWin reads this when it starts, so log out and back in. Then watch what the plugin sees
and does:

```sh
journalctl --user -f | grep -E 'swipe|pinch|switcher|running|config loaded'
```

`states:` in the log is a bit mask: 1 = Overview open, 2 = desktop shown.

* **Is it loaded?**
  `qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadedPlugins`
* **Edited the config but nothing changed:** run `qdbus6 org.kde.KWin /KWin reconfigure`
  (hand edits don't notify the plugin) and look for `config loaded` in the log (debug logging on).
* **A gesture is recognized but nothing happens:** check that the shortcut exists in
  `shortcutNames`. For example, "Switch One Desktop to the Left" does nothing if your
  desktops are stacked in rows. That's why the defaults use Next/Previous Desktop.
* **A 4-finger swipe opens the switcher:** the touchpad reported 3 fingers at the start
  (one finger landed late). The log shows `swipe begin, fingers: N` for every swipe.
* **Alt seems stuck:** tap the physical Alt key once.

---

## Shipping

Binary KWin plugins only load into the **exact KWin version** they were built against, so
this can't ship as a Flatpak, Snap, AppImage or prebuilt `.so`. It ships as source plus
distro packages that rebuild automatically.

### License

GPL-2.0-or-later (required: the plugin links against KWin, which is GPL). See `LICENSE`.
Every source file carries SPDX headers.

### GitHub

* `.github/workflows/build.yml` builds and tests against the newest KWin in an openSUSE
  Tumbleweed container on every push and pull request, and **weekly**, so a Plasma update
  that breaks KWin's internal API shows up as a failed run.
* Releases are git tags `vX.Y.Z`. GitHub's tarball for a tag
  (`archive/vX.Y.Z/kwin-winlike-gestures-X.Y.Z.tar.gz`) is what the RPM builds from.

To release:

1. Bump `VERSION` in `CMakeLists.txt` and `Version:` in `packaging/kwin-winlike-gestures.spec`.
2. Commit, then `git tag vX.Y.Z && git push --tags`.
3. Update the OBS package (below).

### openSUSE Build Service (OBS)

`packaging/kwin-winlike-gestures.spec` builds the plugin, runs the tests in `%check`, and
pins the package to the KWin it was built against (`%requires_eq kwin6` →
`Requires: kwin6 = 6.8.1`). OBS rebuilds it whenever Tumbleweed's `kwin6` changes, so
users always get a matching build through `zypper`.

One-time setup (needs an account on build.opensuse.org and `sudo zypper in osc`):

```sh
osc checkout home:<you>                       # your home project
cd home:<you>
osc mkpac kwin-winlike-gestures && cd kwin-winlike-gestures
cp ~/kde-gestures/packaging/kwin-winlike-gestures.spec ~/kde-gestures/packaging/_service .
osc add kwin-winlike-gestures.spec _service   # _service fetches Source0 from GitHub
osc commit -m "Initial package"
```

Then in the web UI add the **openSUSE Tumbleweed** repository to the project. For each
release, update `Version:` in the spec, `osc commit`, and OBS fetches the new tarball and
rebuilds.

Users install with (the published package lives at
[home:MetuMortis/kwin-winlike-gestures](https://build.opensuse.org/package/show/home:MetuMortis/kwin-winlike-gestures),
built for Tumbleweed and Slowroll, x86_64):

```sh
sudo zypper addrepo https://download.opensuse.org/repositories/home:MetuMortis/openSUSE_Tumbleweed/home:MetuMortis.repo
sudo zypper in kwin-winlike-gestures
```

(Slowroll: replace `openSUSE_Tumbleweed` with `openSUSE_Slowroll` in both places.)

To test the spec locally before uploading:

```sh
mkdir -p /tmp/rpm/SOURCES
git archive --prefix=kwin-winlike-gestures-0.1.0/ -o /tmp/rpm/SOURCES/kwin-winlike-gestures-0.1.0.tar.gz HEAD
rpmbuild -bb --define "_topdir /tmp/rpm" packaging/kwin-winlike-gestures.spec
```

### Other distros

The CMake install is portable (`KDE_INSTALL_USE_QT_SYS_PATHS` puts files in each
distro's Qt plugin directory), and `install.sh` asks CMake for the paths. A distro package
must depend on the exact KWin version it was built with: an Arch `PKGBUILD` for the AUR or
a Fedora COPR would follow the same pattern as the spec.

---

## Known limitations / ideas

* **3-finger tap → search** isn't implemented. libinput reports it as a middle-click tap,
  not as a gesture.
* **Watching mode** (only some directions of a finger count bound) can't always stop
  KWin's built-in animation once it has started.
* The live switcher fakes Alt/Tab/arrow keys. If you've remapped Walk Through Windows
  away from Alt+Tab, set the gesture to a `shortcut:` instead.
* The settings page is English-only (strings are marked for translation, but no
  translations are shipped).

## Prior art

[InputActions](https://github.com/taj-ny/InputActions) and
[kwin-gestures](https://github.com/gbytedev/kwin-gestures) solve the same problem with
KWin plugins and were used as references for KWin's internal API.
