# Conditional Gestures Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the fixed "While …" settings sections with user-created conditional gestures (one Kirigami card per situation on a sub-page), add a "Press key" action, and ship the desktop-grid situation with built-in arrow/Enter rules.

**Architecture:** Config format is unchanged (`Swipe3Left.Grid=key:Left`). Decision logic (new `Key` action kind, grid defaults, which keys are rules) lives in `winlike_core` with unit tests. The `.so` only learns to inject a key combination via KWin's `Xkb` + `processKey`. The KCM gets a thin rule API over core helpers; QML gets a sub-page (`kcm.push`) of `Kirigami.Card`s.

**Tech Stack:** C++23, Qt6 (Core/Gui/Quick), KF6 KCMUtils, Kirigami, `org.kde.kquickcontrols` (`KeySequenceItem`), KWin 6.8.1 internal API, QtTest.

**Spec:** `docs/superpowers/specs/2026-11-02-conditional-gestures-design.md`

## Global Constraints

- `src/recognizer.*`, `src/gestureconfig.*` (`winlike_core`): **QtCore only, no KWin, no QtGui.** Tests first, in `tests/test_gestureconfig.cpp`.
- Never make a blocking D-Bus call from the plugin (`.so`). The KCM may block.
- Every fake key pressed in KWin must be released on every path.
- New files get the SPDX header used everywhere (`GPL-2.0-or-later`, `LiberaTeMetuMortis`).
- Style: KDE/Qt naming, `m_` members, `QStringLiteral`, short "why" comments, `qCDebug(WINLIKE)` / `qCWarning(WINLIKE)`.
- Config keys keep today's format (state suffixes `.Overview`, `.Grid`, `.Desktop`; `inherit` = no rule). Breaking it is allowed but only if needed, and must be called out.
- Commits: on branch `conditional-gestures`, **no `Co-Authored-By: Claude` line** (user memory).
- Build/test: `cmake --build build -j$(nproc) && ctest --test-dir build --output-on-failure`. Never run `sudo`; installs are the user's `./install.sh` in Konsole, plugin changes need a re-login.
- README must match behaviour, defaults and config keys after every user-visible change.

## Review Focus

1. A rule whose action equals the normal gesture's action (e.g. Overview card, 3 left → Window switcher) must survive save + reload, not vanish. → Task 3 test `ruleEqualToNormalIsKept`.
2. A deleted built-in rule must stay deleted after reload; a deleted user rule must leave no stray `inherit` in the file. → Task 3 test `overridesDropsStrayInherit`.
3. "Press key" with a key the current layout can't produce, or that needs Shift (e.g. `Ctrl+!`), must not leave a modifier held. → Task 2 step "unmappable key" (manual journal check; the `.so` has no unit tests).
4. Opening the grid with KWin's follow-the-fingers gesture (`plasma:4Down`, the user's `Swipe3Up`) must give `states: 4` on the next swipe. → Task 2 manual checkpoint.
5. Mid-swipe "swipe back to undo" after a Press key action must not repeat or undo anything (`canUndo()` false). → Task 1 test `keyCannotUndo`.

---

### Task 0: Branch

- [ ] **Step 1:** `git switch -c conditional-gestures` (carries the uncommitted grid work: `State::GridOpen`, detection in `currentStates()`, grid tests, README, and the temporary "While the desktop grid is open" QML section that Task 6 removes).
- [ ] **Step 2:** Add `.superpowers/` to `.gitignore`. Commit spec + grid work: `git add -A && git commit -m "Desktop grid state and conditional gestures design"`.

---

### Task 1: `key:` action in core

**Files:**
- Modify: `src/gestureconfig.h` (enum `Action::Kind`, `canUndo`)
- Modify: `src/gestureconfig.cpp` (`Action::parse`, `Action::toString`)
- Modify: `src/actionrunner.cpp` (add `case Action::Kind::Key:` placeholder `break;` so the switch compiles; Task 2 fills it)
- Test: `tests/test_gestureconfig.cpp`

**Interfaces:**
- Produces: `Action::Kind::Key`; for it, `Action::name` holds the key text in `QKeySequence::PortableText` form (e.g. `"Ctrl+Shift+T"`); spec prefix `key:`.

- [ ] **Step 1: Write failing tests**

```cpp
void parseKey()
{
    const Action a = Action::parse(QStringLiteral("key:Ctrl+Shift+T"));
    QCOMPARE(a.kind, Action::Kind::Key);
    QCOMPARE(a.name, QStringLiteral("Ctrl+Shift+T"));
    QCOMPARE(a.toString(), QStringLiteral("key:Ctrl+Shift+T"));
    QVERIFY(a.handled());
    QCOMPARE(Action::parse(QStringLiteral(" key:Left ")).name, QStringLiteral("Left"));
    // Empty key: nothing to press, treat as passthrough.
    QVERIFY(!Action::parse(QStringLiteral("key:")).handled());
}
void keyCannotUndo()
{
    QVERIFY(!Action::parse(QStringLiteral("key:Left")).canUndo());
}
```

- [ ] **Step 2:** Run tests → FAIL (`Key` not a member of `Action::Kind`).
- [ ] **Step 3:** Implement: `s_keyPrefix = "key:"`; `parse` → `{Kind::Key, {}, text}` when text non-empty after trimming, else passthrough; `toString` → `"key:" + name`; `canUndo` unchanged (already excludes it). Add the `Key` case to `runAction`'s switch with `break;`.
- [ ] **Step 4:** Run tests → PASS.
- [ ] **Step 5:** Commit `"Press key action in config"`.

---

### Task 2: Inject keys in KWin + built-in grid rules

**Files:**
- Modify: `src/actionrunner.cpp` (implement `Kind::Key`)
- Modify: `src/gestureconfig.cpp` (`defaults()`)
- Modify: `tests/test_gestureconfig.cpp` (`gridDefaults` from the grid work)
- Modify: `README.md` (default gestures table, config example)

**Interfaces:**
- Consumes: `Action::Kind::Key`, `Action::name` (Task 1).
- Produces: new built-in defaults used by Tasks 3–6:
  `Swipe3Left.Grid=key:Left`, `Swipe3Right.Grid=key:Right`, `Swipe3Down.Grid=key:Return`, `Swipe4Down.Grid=shortcut:kwin/Grid View` (and **no** `Swipe3Up.Grid` / `Swipe4*` sideways).

- [ ] **Step 1: Update failing test `gridDefaults`** (replace the existing body):

```cpp
GestureConfig c;
const auto grid = [&](int f, Direction d) { return c.action(GestureType::Swipe, f, d, State::GridOpen); };
QCOMPARE(grid(3, Direction::Left).kind, Action::Kind::Key);
QCOMPARE(grid(3, Direction::Left).name, QStringLiteral("Left"));
QCOMPARE(grid(3, Direction::Right).name, QStringLiteral("Right"));
QCOMPARE(grid(3, Direction::Down).name, QStringLiteral("Return"));
QCOMPARE(grid(4, Direction::Down).name, QStringLiteral("Grid View"));
QCOMPARE(grid(3, Direction::Up).name, QStringLiteral("Overview")); // no rule: normal key
// All four 3-finger directions are ours in the grid, so the switcher never opens there.
QVERIFY(c.handlesAllDirections(GestureType::Swipe, 3, State::GridOpen));
QCOMPARE(c.action(GestureType::Swipe, 3, Direction::Left).kind, Action::Kind::Switcher);
```

  Also fix `gridOverridesAndInherit`'s `inherit` assertion if it depended on the old `Swipe3Down.Grid` value (after `inherit`, 3 down in grid → `"Show Desktop"` still holds).
- [ ] **Step 2:** Run → FAIL.
- [ ] **Step 3:** Change `defaults()` to the four grid entries above (drop `Swipe3Down.Grid=Grid View`).
- [ ] **Step 4:** Run → PASS.
- [ ] **Step 5: Implement `Kind::Key` in `runAction`** as a file-local `void pressKey(const QString &text)`:
  - `QKeySequence::fromString(text, QKeySequence::PortableText)`; empty → `qCWarning`, return. Use `seq[0]` (a `QKeyCombination`).
  - Keysyms: `KWin::Xkb::keysymsFromQtKey(combination.key())`-style call (it takes a `QKeyCombination`; pass the key without modifiers); for the first keysym where `KWin::input()->keyboard()->xkb()->keycodeFromKeysym(sym)` has a value, use it. None → `qCWarning(WINLIKE) << "no key for" << text`, return without pressing anything.
  - Evdev code = `keyCode - 8`. Modifiers from `combination.keyboardModifiers()`: Ctrl→`KEY_LEFTCTRL`, Shift→`KEY_LEFTSHIFT`, Alt→`KEY_LEFTALT`, Meta→`KEY_LEFTMETA`; additionally press `KEY_LEFTSHIFT` if `KeyCode::level > 0` and Shift isn't already in the list (keys like `!` on level 2).
  - Order: press modifiers, press key, release key, release modifiers in reverse — all with the same `std::chrono` steady-clock timestamp, via `processKey(code, KWin::KeyboardKeyState::Pressed/Released, time)`. All presses and releases happen in this one function, so nothing can stay held.
  - Includes: `<keyboard_input.h>`, `<xkb.h>`, `<input.h>`, `<QKeySequence>`, `<linux/input-event-codes.h>`.
- [ ] **Step 6:** Build + tests pass. Update README: default table row for the grid ("**If the desktop grid is open:** 3-finger left/right move the highlight, 3-finger down opens the highlighted window, 4-finger down closes the grid"), add `key:` to the actions table and `Swipe3Left.Grid=key:Left` to the config example.
- [ ] **Step 7:** Commit `"Press key runs in KWin; built-in grid rules"`.
- [ ] **Step 8: Manual checkpoint (user).** Ask the user to run `./install.sh` and re-login, then:
  1. Open the grid with Meta+G, swipe 3 left/right/down → highlight moves, Enter opens the window. Journal: `journalctl --user --since "-2min" -o cat | grep -E 'states|running|no key'` shows `states: 4` and `running "key:Left"`.
  2. Same after opening the grid with their 3-finger up (`plasma:4Down`).
  3. Unmappable key (Review Focus 3): `kwriteconfig6 --notify --file kwinwinlikegesturesrc --group Gestures --key Swipe4Up.Grid "key:Ctrl+F35"`, swipe 4 up in grid → `no key for` warning, then type normally (no stuck Ctrl). Remove the key afterwards.
  If (1)/(2) fail, fix `currentStates()` detection before Task 3.

---

### Task 3: Rule helpers in core

**Files:**
- Modify: `src/gestureconfig.h`, `src/gestureconfig.cpp`
- Test: `tests/test_gestureconfig.cpp`

**Interfaces:**
- Produces (all `static` on `GestureConfig`):
  - `QStringList gestureKeys()` — base keys in display order: `Swipe3Up, Swipe3Down, Swipe3Left, Swipe3Right, Swipe4Up, Swipe4Down, Swipe4Left, Swipe4Right, Pinch2In, Pinch2Out, Pinch3In, Pinch3Out`.
  - `QStringList situations()` — suffixes in precedence/card order: `".Grid", ".Overview", ".Desktop"`.
  - `QStringList ruleKeys(const QString &suffix, const QMap<QString, QString> &overrides)` — full keys (`"Swipe3Left.Grid"`) that are rules: effective value (override, else built-in default) exists and isn't `inherit`; ordered by `gestureKeys()`.
  - `overrides()` gains: drop an `inherit` value on a key that has no built-in default (it means nothing).

- [ ] **Step 1: Failing tests**

```cpp
void ruleKeysDefaults()
{
    QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Grid"), {}),
             (QStringList{QStringLiteral("Swipe3Down.Grid"), QStringLiteral("Swipe3Left.Grid"),
                          QStringLiteral("Swipe3Right.Grid"), QStringLiteral("Swipe4Down.Grid")}));
    QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Overview"), {}).size(), 2);
    QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Desktop"), {}).size(), 2);
}
void ruleKeysOverrides()
{
    const QMap<QString, QString> o{{QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")},
                                   {QStringLiteral("Pinch2In.Overview"), QStringLiteral("block")}};
    QCOMPARE(GestureConfig::ruleKeys(QStringLiteral(".Overview"), o),
             (QStringList{QStringLiteral("Swipe4Down.Overview"), QStringLiteral("Pinch2In.Overview")}));
}
void ruleEqualToNormalIsKept()
{
    const QMap<QString, QString> e{{QStringLiteral("Swipe3Left.Overview"), QStringLiteral("switcher")}};
    QCOMPARE(GestureConfig::overrides(e), e);
    QVERIFY(GestureConfig::ruleKeys(QStringLiteral(".Overview"), e).contains(QStringLiteral("Swipe3Left.Overview")));
}
void overridesDropsStrayInherit()
{
    const QMap<QString, QString> e{{QStringLiteral("Swipe3Left.Overview"), QStringLiteral("inherit")},
                                   {QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")}};
    // Built-in rule deleted: kept. No built-in: meaningless, dropped.
    QCOMPARE(GestureConfig::overrides(e), (QMap<QString, QString>{{QStringLiteral("Swipe3Down.Overview"), QStringLiteral("inherit")}}));
}
```

- [ ] **Step 2:** Run → FAIL. **Step 3:** Implement. **Step 4:** Run → PASS.
- [ ] **Step 5:** Commit `"Core helpers for conditional rules"`.

---

### Task 4: KCM rule API

**Files:**
- Modify: `kcm/kcm_winlikegestures.h`, `kcm/kcm_winlikegestures.cpp`

**Interfaces:**
- Consumes: `GestureConfig::gestureKeys()`, `situations()`, `ruleKeys()` (Task 3).
- Produces (QML-callable on `kcm`; all re-evaluate through `kcm.revision`):
  - `Q_INVOKABLE QStringList gestureKeys() const`, `Q_INVOKABLE QStringList situations() const` (forwarders).
  - `Q_INVOKABLE QStringList rules(const QString &suffix) const` → `ruleKeys(suffix, m_overrides)`.
  - `Q_INVOKABLE int ruleCount() const` → total over all situations.
  - `Q_INVOKABLE void addRule(const QString &key)` → `setValue(key, "block")`.
  - `Q_INVOKABLE void removeRule(const QString &key)` → `setValue(key, "inherit")`.
  - `Q_INVOKABLE void removeSituation(const QString &suffix)` → `removeRule` on each of `rules(suffix)`.
  - `Q_INVOKABLE QKeySequence keySequence(const QString &key) const` → parses `value(key)` after `key:`; and `Q_INVOKABLE void setKeySequence(const QString &key, const QKeySequence &seq)` → `setValue(key, "key:" + seq.toString(QKeySequence::PortableText))`, or `removeRule`-equivalent `"block"` if `seq` is empty.

- [ ] **Step 1:** Change `defaultValue()`: a state key without a built-in default now returns `"inherit"` (was: the normal gesture's value); `value()` returns `"inherit"` for it too (drop the fall-through to `value(base)`). Update the header comment on `value()`.
- [ ] **Step 2:** Add the methods above. `setValue`'s "equal to default → remove override" rule then gives the right file contents: deleting a user rule removes the entry, deleting a built-in writes `inherit`, adding writes `block`.
- [ ] **Step 3:** Build; `ctest` passes. Smoke test from the build dir (`QT_PLUGIN_PATH=$PWD/build/bin:$QT_PLUGIN_PATH kcmshell6 kcm_winlikegestures`): page still opens (old state rows now show "Leave to Plasma or the app" for unset keys — expected until Task 6).
- [ ] **Step 4:** Commit `"KCM: rule list API"`.

---

### Task 5: "Press key…" in every gesture row

**Files:**
- Modify: `kcm/ui/GestureRow.qml`
- Modify: `packaging/kwin-winlike-gestures.spec` (`Requires: kf6-kdeclarative-imports`)

**Interfaces:**
- Consumes: `kcm.keySequence(key)`, `kcm.setKeySequence(key, seq)` (Task 4).
- Produces: `GestureRow` handles `key:` specs; also used inside cards in Task 6.

- [ ] **Step 1:** Add preset `{ text: i18nc("@item:inlistbox", "Press key…"), spec: "", custom: "key" }` after "Run command…". `customKind` returns `"key"` for `key:` specs. Choosing it writes `kcm.setValue(row.key, "key:Return")` as a starting value (a bare `key:` parses as passthrough and would hide the recorder).
- [ ] **Step 2:** Add `KQC.KeySequenceItem` (`import org.kde.kquickcontrols as KQC`), visible when `customKind === "key"`, `keySequence: kcm.keySequence(row.key)` (bound via `kcm.revision`), `modifierlessAllowed: true`, `modifierOnlyAllowed: false`, `checkForConflictsAgainst: KQC.ShortcutType.None`, `onCaptureFinished: kcm.setKeySequence(row.key, keySequence)`.
- [ ] **Step 3:** Add `.Grid` to `stateNames`: `"shortcut:kwin/Grid View"` → "Close desktop grid" (already in the uncommitted work; keep).
- [ ] **Step 4:** Check from the build dir: on "Three-finger swipe → Left" pick Press key…, record Ctrl+Shift+T, Apply; `grep Swipe3Left ~/.config/kwinwinlikegesturesrc` shows `Swipe3Left=key:Ctrl+Shift+T`. Record a bare Left: accepted (no "needs modifier" refusal). Restore with Defaults → Apply. Screenshot with `spectacle -b -n -a -o <scratchpad>/kcm-key.png` while the KCM window is focused and look at it.
- [ ] **Step 5:** Commit `"Settings: Press key action with key recorder"`.

---

### Task 6: Conditional gestures sub-page

**Files:**
- Create: `kcm/ui/ConditionalGestures.qml` (picked up by `kcmutils_add_qml_kcm`; nothing to add to `install()`/`%files`; verify `build/installed-files.txt` doesn't list QML files individually)
- Modify: `kcm/ui/main.qml`

**Interfaces:**
- Consumes: `kcm.situations()`, `kcm.rules(suffix)`, `kcm.ruleCount()`, `kcm.gestureKeys()`, `kcm.addRule/removeRule/removeSituation`, `GestureRow` (Task 5).

- [ ] **Step 1: `main.qml`:** delete the three "While …" sections (Overview, desktop grid, desktop shown). Add section "Conditional gestures" with a row labelled "Special situations:": `QQC2.Label` showing `i18ncp("@info", "%1 rule", "%1 rules", kcm.ruleCount())` (or "None" at 0) and `QQC2.Button { text: i18nc("@action:button", "Edit…"); icon.name: "document-edit"; onClicked: kcm.push("ConditionalGestures.qml") }`, plus a `ContextualHelpButton` explaining that these gestures replace the normal ones in a situation, and gestures without a rule there do their normal action. Bind count through `kcm.revision`.
- [ ] **Step 2: `ConditionalGestures.qml`** — `KCM.ScrollViewKCM` (or `SimpleKCM` with a `ColumnLayout`), `title: i18nc("@title", "Conditional Gestures")`:
  - `actions: [Kirigami.Action { text: "Add New"; icon.name: "list-add"; children: one Kirigami.Action per situation, enabled when the situation has no card }]`.
  - Cards: `Repeater` over `kcm.situations()`, a `Kirigami.Card` shown when `kcm.rules(suffix).length > 0` or the suffix is in a page-local `property var openedEmpty: []` (filled by Add New; not saved). Header: `Kirigami.Heading` level 3 + `Kirigami.Icon`. Titles/icons: `.Grid` "While the desktop grid is open" / `view-grid`; `.Overview` "While Overview is open" / `view-app-grid-symbolic` (builder may pick better Breeze names); `.Desktop` "While the desktop is shown" / `user-desktop`.
  - Card body: per key in `kcm.rules(suffix)`: `QQC2.Label` with the gesture name, a `GestureRow { key: modelData }`, `QQC2.ToolButton { icon.name: "edit-delete"; onClicked: kcm.removeRule(modelData) }`. Gesture names: `Swipe<n><Dir>` → i18nc "%1 fingers up/down/left/right", `Pinch<n>In/Out` → "%1-finger pinch in/out".
  - `Card.actions`: "Add gesture" with child actions for each `kcm.gestureKeys()` entry whose `key + suffix` isn't already a rule → `kcm.addRule(key + suffix)`; "Remove situation" → `kcm.removeSituation(suffix)` and drop it from `openedEmpty`.
  - `Kirigami.PlaceholderMessage` when no card is visible: text "No special situations", explanation "Click Add New to make gestures do something else while Overview or the desktop grid is open, or while the desktop is shown."
  - Card order = `situations()` order.
- [ ] **Step 3:** Check from the build dir with screenshots of the main page and the sub-page (fresh defaults: three cards, 8 rules — Grid 4, Overview 2, Desktop 2; count on main page reads "8 rules"). Then: add Pinch2In to Overview → shows "Do nothing", Apply, reopen → still there; delete Swipe3Down from Overview → Apply, reopen → still gone, file has `Swipe3Down.Overview=inherit`; Add New when all three exist → all items disabled; Remove situation on Grid → card gone, Apply, file has `inherit` for the 4 grid keys; Defaults → three cards back. Look at the screenshots for layout (no clipped combos at the default window width).
- [ ] **Step 4:** Commit `"Settings: conditional gestures page with a card per situation"`.

---

### Task 7: Docs and wrap-up

**Files:**
- Modify: `README.md` (settings-page section: replace the "While …" dropdown description with the conditional gestures page; state-dependent bindings: rules are per situation, any gesture; built-in rules list; log bitmask already has 4 = grid)
- Modify: `CLAUDE.md` only if a new trap was hit.
- Maybe: `docs/settings-page.png` (README screenshot) — retake from the build dir if the main page changed visibly.

- [ ] **Step 1:** Update README; `grep -n "While Overview is open\|While the desktop" README.md kcm/ui/*.qml` → only intended hits remain.
- [ ] **Step 2:** Full build + `ctest` → all pass.
- [ ] **Step 3:** Commit `"README: conditional gestures"`. Ask the user to `./install.sh`, re-login, and run the Task 2 checkpoint once more plus one card edit from System Settings.
