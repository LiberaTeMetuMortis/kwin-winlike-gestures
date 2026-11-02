# Conditional gestures: design

Date: 2026-11-02. Status: approved direction, awaiting final read.

## Why

The settings page shows one fixed section per desktop state ("While Overview is open",
"While the desktop is shown", and, from the uncommitted work, "While the desktop grid is
open"), each listing the same four up/down rows. Every new state multiplies the page by
another full block of rows, most of which the user never changes. The user wants to
*create* conditional gestures instead: pick a situation, add only the gestures that
should behave differently there.

Their motivating example: while the desktop grid is open, 3-finger left/right moves the
grid's highlight between windows and 3-finger down opens the highlighted window. That
needs a new action that sends keystrokes.

## Goals

1. Replace the fixed per-state sections with user-created conditional gestures, laid out
   as the **hybrid** design chosen from mockups (below).
2. Add a **Press key** action: sends one key combination (e.g. `Left`, `Return`,
   `Ctrl+Shift+T`) to whatever has focus. Available for every gesture, normal or
   conditional.
3. Ship the **desktop grid** condition (already built, uncommitted: `State::GridOpen`,
   `.Grid` suffix, detection in `currentStates()`).
   Its detection (reading the overview effect's `gridPartialActivationFactor` /
   `transitionPartialActivationFactor`) is **not yet tested on the live session**. The
   plan starts with an install + re-login check, including the user's own case of opening
   the grid with `Swipe3Up=plasma:4Down` (KWin's follow-the-fingers grid gesture), and a
   fix if the journal doesn't show `states: 4` there.

## Non-goals

* **No and/or/not operators.** The user considered and explicitly dropped them as
  overkill. A rule has exactly one situation.
* **No new situations** beyond the three: Overview open, desktop grid open, desktop shown
  (user: "those three are enough for now").
* No key *sequences* ("Ctrl+K, then S") and no holding keys for the length of the swipe:
  one combination, pressed and released once.
* No change to how the normal (unconditional) gestures are shown.

## Anti-goals

* A page that again grows a block of rows for every situation × gesture combination.
* Anything that blocks KWin (see CLAUDE.md traps: no sync D-Bus from the plugin).

## The settings page (decided with the user, from mockups)

**Main page** (`kcm/ui/main.qml`, still a `Kirigami.FormLayout`): the three-finger,
four-finger, pinch and sensitivity sections stay as they are. The three "While …"
sections are removed and replaced by one section, "Conditional gestures", with a single
row: label "Special situations:", a count ("4 rules", or "None"), and an **Edit…**
button. Edit pushes a sub-page with `kcm.push(...)`, the way Window Rules and Autostart
open their editors.

**Editor sub-page** (new QML file, `KCM.ScrollViewKCM` or `KCM.SimpleKCM`):

* Title "Conditional Gestures", back arrow from the KCM framework.
* Page action **Add New ▾** (a `Kirigami.Action` with child actions, like Autostart's
  "Add New" menu) listing the three situations. Situations that already have a card are
  disabled.
* One **`Kirigami.Card` per situation** (the user specifically asked for Kirigami cards
  here). Card header: situation name, with an icon. Card body: one row per rule:
  gesture name (label) → action combo box (the same presets/custom fields as
  `GestureRow.qml`, including Press key) → delete `ToolButton` (`edit-delete`). Card
  action bar (`Card.actions`): **Add gesture ▾** (menu listing only gestures this
  situation doesn't have yet) and **Remove situation**.
* Empty page: `Kirigami.PlaceholderMessage`, e.g. "No special situations", explanation
  pointing at Add New.
* Card order is fixed: desktop grid, Overview, desktop shown (the precedence order).

Mockups the user approved are in `.superpowers/brainstorm/*/content/hybrid.html`
(local, not committed).

## Behaviour of rules

* A situation's card can hold any gesture: 3- and 4-finger swipes in all four
  directions, 2- and 3-finger pinch in/out. (Today the state sections offer only
  up/down.) *My call.*
* A gesture with no rule in the active situation does its normal action. Only the
  topmost situation's rules apply: grid, then Overview, then desktop shown (unchanged
  from the current `GestureConfig::action`). *Existing behaviour, kept.*
* **Built-in rules appear as ordinary rules** in their cards (user confirmed: show them
  as cards). They can be edited or deleted; deletion sticks across reloads; the page's
  **Defaults** button brings them back. So a fresh install opens the editor with three
  filled cards, and the empty placeholder only shows if everything is deleted.
* **Built-in rules** (new defaults in `GestureConfig::defaults()`):

  | Situation | Rule | Source |
  |---|---|---|
  | Overview open | 3/4 down → close Overview (`shortcut:kwin/Overview`) | existing |
  | Desktop shown | 3/4 up → bring windows back (`shortcut:kwin/Show Desktop`) | existing |
  | Grid open | 3 left → `key:Left`, 3 right → `key:Right` | user: "3 l/r should just press left and right arrow without pressing alt tab" |
  | Grid open | 3 down → `key:Return` (open the highlighted window) | user's original example; replaces the uncommitted `Swipe3Down.Grid=Grid View` |
  | Grid open | 4 down → close the grid (`shortcut:kwin/Grid View`) | uncommitted work, kept |

  Grid left/right must be Owned from the first event (all four 3-finger directions are
  bound in that state), so the Alt+Tab switcher never opens in the grid.
* A newly added rule starts as **Do nothing** (`block`) so it is visibly a rule and is
  saved. *My call.*
* "Add New" with a situation creates an empty card in the page only; nothing is written
  until a gesture is added. An empty card is not saved, so it is gone the next time the
  page opens. *My call.*
* **Defaults** is the KCM's standard whole-module button: pressed on the sub-page it
  resets everything (normal gestures and tuning too), as on any KDE settings page. No
  separate "restore built-in rules" button. *My call: standard KCM behaviour.*
* **Remove situation** deletes every rule in that card (user rules removed, built-in ones
  written as `inherit`).

## Config format (unchanged)

Keep `~/.config/kwinwinlikegesturesrc` `[Gestures]` keys with state suffixes:
`Swipe3Down.Overview`, `Swipe3Down.Grid`, `Swipe3Up.Desktop`, `Pinch2In.Grid`, … A card
is simply the set of keys with that suffix that have a binding. `inherit` keeps meaning
"no rule here" (needed to delete a built-in rule). Existing files keep working.
User: breaking the config file would be acceptable, but keeping the same format is
preferred; so keep it unless it gets in the way, and say so if it does.

KCM model change this needs: today an unset state key *displays* its normal gesture's
action (`defaultValue()` falls back to `value(base)`), and `setValue` drops a value equal
to that. Under the new design an unset state key with no built-in default means **no
rule**. The KCM needs to expose, per situation, the list of rule keys (effective binding
≠ `inherit`), add/remove operations, and a rule count for the main page row; and must
stop treating "equal to the normal gesture" as "no rule". `GestureConfig::overrides()`
already keeps state keys equal to their normal gesture, which fits.

## Press key action

* Config spec: `key:<sequence>` in `QKeySequence::PortableText`, e.g. `key:Left`,
  `key:Return`, `key:Ctrl+Shift+T`. Only the first combination of the sequence is used.
* Parsing in `winlike_core` stays string-only (QtCore, no QtGui): `Action::Kind::Key`
  with the text in `name`. `canUndo()` is false (a keystroke can't be undone by swiping
  back). Tests in `tests/test_gestureconfig.cpp`, written first.
* Running it lives in the `.so`: turn the text into a `QKeyCombination`, map the key with
  `KWin::Xkb::keysymsFromQtKey()` and `xkb()->keycodeFromKeysym()` to a keycode
  (xkb keycode − 8 = evdev code), then via `input()->keyboard()->processKey`: press
  modifiers (evdev `KEY_LEFTCTRL`, `KEY_LEFTSHIFT`, `KEY_LEFTALT`, `KEY_LEFTMETA`), press
  and release the key, release modifiers in reverse. Same mechanism the switcher already
  uses for its fake Alt. If a key can't be mapped, `qCWarning` and do nothing.
* Settings page: action preset "Press key…" with a `KeySequenceItem` from
  `org.kde.kquickcontrols` (as KWin's Window Rules uses), shown like the existing
  shortcut picker / command field. New runtime dependency: openSUSE package
  `kf6-kdeclarative-imports` → add to the spec's `Requires`.
* **Checked by the user (2026-11-02):** on the real keyboard, in the desktop grid, the
  arrow keys move the highlight and Enter (`Return`) opens the highlighted window. Still
  to confirm after building: the same holds for keys injected with `processKey` (they go
  through KWin's input filters like real keys, so this is expected). The built-in grid
  rules depend on it, so this is checked in the same re-login test as the grid detection.

## Constraints (from CLAUDE.md and the code)

* Decision logic in `winlike_core` with tests first; the `.so` stays thin.
* New config keys/actions: rows/presets in the KCM; README and the README config example
  updated; `GestureRow.qml` gets the Press key preset.
* New installed files (the sub-page QML is compiled into the KCM, so probably none) go in
  `install(...)`, `build/installed-files.txt` and the spec's `%files`.
* The KCM may block on D-Bus; the plugin must not.
* Plugin changes need a re-login to test; the KCM can be tried from the build folder.

## Left to the builder

* Exact QML structure, file names, and how the KCM exposes rules (Q_INVOKABLEs vs. a
  model), as long as the page matches the layout above.
* Icons for the situations.
* Wording of labels and placeholder text (KDE HIG, `i18nc` contexts as in existing QML).
* Whether "Edit…" count includes built-in rules (it should count whatever the editor
  shows).
