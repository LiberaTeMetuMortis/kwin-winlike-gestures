/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

// One gesture binding: a choice of action, plus a shortcut picker or command
// field when the action needs one. Reads and writes kcm.value(key).
RowLayout {
    id: row

    required property string key
    // State rows ("Swipe3Down.Overview") get a "Same as usual" choice.
    readonly property bool isStateRow: key.indexOf(".") !== -1
    readonly property string spec: (kcm.revision, kcm.value(key))

    readonly property var presets: [
        { text: i18nc("@item:inlistbox", "Overview"), spec: "shortcut:kwin/Overview" },
        { text: i18nc("@item:inlistbox", "Show desktop"), spec: "shortcut:kwin/Show Desktop" },
        { text: i18nc("@item:inlistbox", "Window switcher (hold and slide)"), spec: "switcher" },
        { text: i18nc("@item:inlistbox", "Next virtual desktop"), spec: "shortcut:kwin/Switch to Next Desktop" },
        { text: i18nc("@item:inlistbox", "Previous virtual desktop"), spec: "shortcut:kwin/Switch to Previous Desktop" },
        { text: i18nc("@item:inlistbox", "Desktop grid"), spec: "shortcut:kwin/Grid View" },
        { text: i18nc("@item:inlistbox", "Other KWin shortcut…"), spec: "", custom: "shortcut" },
        { text: i18nc("@item:inlistbox", "Run command…"), spec: "", custom: "command" },
        { text: i18nc("@item:inlistbox", "Do nothing"), spec: "block" },
        { text: i18nc("@item:inlistbox", "Leave to Plasma or the app"), spec: "passthrough" },
    ]
    // Overview and Show Desktop toggle, so in the matching state they undo it:
    // name them for what they do there.
    readonly property var stateNames: key.endsWith(".Overview")
        ? { "shortcut:kwin/Overview": i18nc("@item:inlistbox", "Close Overview") }
        : key.endsWith(".Desktop")
            ? { "shortcut:kwin/Show Desktop": i18nc("@item:inlistbox", "Bring windows back") }
            : {}
    readonly property var choices: {
        const renamed = presets.map(c => stateNames[c.spec] ? Object.assign({}, c, { text: stateNames[c.spec] }) : c);
        return isStateRow
            ? [{ text: i18nc("@item:inlistbox", "Same as usual"), spec: "inherit" }].concat(renamed)
            : renamed;
    }

    readonly property string customKind: {
        if (choices.some(c => c.spec === spec)) {
            return "";
        }
        if (spec.startsWith("shortcut:")) {
            return "shortcut";
        }
        if (spec.startsWith("command:")) {
            return "command";
        }
        return "";
    }

    function indexForSpec() {
        for (let i = 0; i < choices.length; ++i) {
            if (customKind !== "" ? choices[i].custom === customKind : choices[i].spec === spec) {
                return i;
            }
        }
        return -1;
    }

    enabled: kcm.pluginEnabled
    spacing: Kirigami.Units.smallSpacing

    QQC2.ComboBox {
        id: actionBox
        Layout.preferredWidth: Kirigami.Units.gridUnit * 16
        textRole: "text"
        model: row.choices
        currentIndex: row.indexForSpec()
        onActivated: index => {
            const choice = row.choices[index];
            if (choice.custom === "shortcut") {
                kcm.setValue(row.key, "shortcut:kwin/" + (kcm.kwinShortcuts[0] ?? "Overview"));
            } else if (choice.custom === "command") {
                kcm.setValue(row.key, "command:");
            } else {
                kcm.setValue(row.key, choice.spec);
            }
        }
    }

    QQC2.ComboBox {
        visible: row.customKind === "shortcut"
        Layout.preferredWidth: Kirigami.Units.gridUnit * 16
        model: kcm.kwinShortcuts
        currentIndex: visible ? model.indexOf(row.spec.slice("shortcut:kwin/".length)) : -1
        onActivated: index => kcm.setValue(row.key, "shortcut:kwin/" + model[index])
    }

    QQC2.TextField {
        visible: row.customKind === "command"
        Layout.preferredWidth: Kirigami.Units.gridUnit * 16
        placeholderText: i18nc("@info:placeholder", "Command, e.g. dolphin")
        text: visible ? row.spec.slice("command:".length) : ""
        onTextEdited: kcm.setValue(row.key, "command:" + text)
    }
}
