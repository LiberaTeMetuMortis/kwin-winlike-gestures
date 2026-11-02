/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root

    // Tuning sliders store numbers as strings in the config file.
    component TuningSlider: RowLayout {
        id: tuning
        required property string key
        property real from
        property real to
        property real stepSize
        property int decimals: 0
        property string lowText
        property string highText
        property string helpText
        // Greys out the slider but not the help icon, so it can still be read.
        property bool active: kcm.pluginEnabled

        QQC2.Label { text: tuning.lowText; enabled: tuning.active }
        QQC2.Slider {
            enabled: tuning.active
            Layout.preferredWidth: Kirigami.Units.gridUnit * 12
            from: tuning.from
            to: tuning.to
            stepSize: tuning.stepSize
            value: (kcm.revision, Number(kcm.value(tuning.key)))
            onMoved: kcm.setValue(tuning.key, value.toFixed(tuning.decimals))
        }
        QQC2.Label { text: tuning.highText; enabled: tuning.active }
        Kirigami.ContextualHelpButton {
            visible: tuning.helpText !== ""
            toolTipText: tuning.helpText
        }
    }

    header: Kirigami.InlineMessage {
        visible: !kcm.pluginEnabled
        type: Kirigami.MessageType.Information
        position: Kirigami.InlineMessage.Position.Header
        text: i18nc("@info", "Gestures are off. Plasma's own touchpad gestures are used instead.")
    }

    Kirigami.FormLayout {
        RowLayout {
            Kirigami.FormData.label: i18nc("@label", "Touchpad gestures:")
            QQC2.Switch {
                text: i18nc("@option:check", "Enabled")
                checked: kcm.pluginEnabled
                onToggled: kcm.pluginEnabled = checked
            }
            Kirigami.ContextualHelpButton {
                toolTipText: i18nc("@info:tooltip", "Turns the gestures on this page on or off, right away and at every login. When off, Plasma's own touchpad gestures are used.")
            }
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Three-finger swipe")
        }
        GestureRow { key: "Swipe3Up"; Kirigami.FormData.label: i18nc("@label", "Up:") }
        GestureRow { key: "Swipe3Down"; Kirigami.FormData.label: i18nc("@label", "Down:") }
        GestureRow {
            key: "Swipe3Left"
            Kirigami.FormData.label: i18nc("@label", "Left:")
            helpText: i18nc("@info:tooltip", "Window switcher: opens the Alt+Tab switcher and keeps it open while your fingers stay down. Keep sliding left, right, up or down to move the selection, then lift to switch. A quick flick jumps to the previous window.")
        }
        GestureRow { key: "Swipe3Right"; Kirigami.FormData.label: i18nc("@label", "Right:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Four-finger swipe")
        }
        GestureRow { key: "Swipe4Up"; Kirigami.FormData.label: i18nc("@label", "Up:") }
        GestureRow { key: "Swipe4Down"; Kirigami.FormData.label: i18nc("@label", "Down:") }
        GestureRow {
            key: "Swipe4Left"
            Kirigami.FormData.label: i18nc("@label", "Left:")
            helpText: i18nc("@info:tooltip", "The \"Slide to the desktop…\" choices use Plasma's own animation, which follows your fingers: the desktop moves with them, so swiping left brings in the desktop on the right. Stop and swipe back before lifting to stay where you are. A slide only works if there is a desktop on that side; with desktops in rows, use \"below\" and \"above\" for up and down.")
        }
        GestureRow { key: "Swipe4Right"; Kirigami.FormData.label: i18nc("@label", "Right:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "While Overview is open")
        }
        GestureRow {
            key: "Swipe3Up.Overview"
            Kirigami.FormData.label: i18nc("@label", "Three fingers up:")
            helpText: i18nc("@info:tooltip", "These replace the normal gestures while Overview is open. Rows you haven't changed follow the normal gesture above. Overview opens and closes with the same action, so in this section it shows as \"Close Overview\".")
        }
        GestureRow { key: "Swipe3Down.Overview"; Kirigami.FormData.label: i18nc("@label", "Three fingers down:") }
        GestureRow { key: "Swipe4Up.Overview"; Kirigami.FormData.label: i18nc("@label", "Four fingers up:") }
        GestureRow { key: "Swipe4Down.Overview"; Kirigami.FormData.label: i18nc("@label", "Four fingers down:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "While the desktop grid is open")
        }
        GestureRow {
            key: "Swipe3Up.Grid"
            Kirigami.FormData.label: i18nc("@label", "Three fingers up:")
            helpText: i18nc("@info:tooltip", "These replace the normal gestures while the desktop grid is open. Rows you haven't changed follow the normal gesture above, not the Overview section. The desktop grid opens and closes with the same action, so in this section it shows as \"Close desktop grid\".")
        }
        GestureRow { key: "Swipe3Down.Grid"; Kirigami.FormData.label: i18nc("@label", "Three fingers down:") }
        GestureRow { key: "Swipe4Up.Grid"; Kirigami.FormData.label: i18nc("@label", "Four fingers up:") }
        GestureRow { key: "Swipe4Down.Grid"; Kirigami.FormData.label: i18nc("@label", "Four fingers down:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "While the desktop is shown")
        }
        GestureRow {
            key: "Swipe3Up.Desktop"
            Kirigami.FormData.label: i18nc("@label", "Three fingers up:")
            helpText: i18nc("@info:tooltip", "These replace the normal gestures while the desktop is shown (after Show desktop). Rows you haven't changed follow the normal gesture above. Show desktop switches on and off with the same action, so in this section it shows as \"Bring windows back\", which restores the windows exactly as they were.")
        }
        GestureRow { key: "Swipe3Down.Desktop"; Kirigami.FormData.label: i18nc("@label", "Three fingers down:") }
        GestureRow { key: "Swipe4Up.Desktop"; Kirigami.FormData.label: i18nc("@label", "Four fingers up:") }
        GestureRow { key: "Swipe4Down.Desktop"; Kirigami.FormData.label: i18nc("@label", "Four fingers down:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Pinch")
        }
        GestureRow {
            key: "Pinch2In"
            Kirigami.FormData.label: i18nc("@label", "Two fingers in:")
            helpText: i18nc("@info:tooltip", "Browsers, image viewers and other apps use a two-finger pinch to zoom. Binding it here takes it away from them, so it's best left to the app.")
        }
        GestureRow { key: "Pinch2Out"; Kirigami.FormData.label: i18nc("@label", "Two fingers out:") }
        GestureRow {
            key: "Pinch3In"
            Kirigami.FormData.label: i18nc("@label", "Three fingers in:")
            helpText: i18nc("@info:tooltip", "Left to Plasma, a three-finger pinch zooms the whole screen (the Zoom accessibility effect). Bind both directions to use it for something else instead.")
        }
        GestureRow { key: "Pinch3Out"; Kirigami.FormData.label: i18nc("@label", "Three fingers out:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Sensitivity")
        }
        RowLayout {
            Kirigami.FormData.label: i18nc("@label:listbox", "Run actions:")
            QQC2.ComboBox {
                readonly property var choices: [
                    { text: i18nc("@item:inlistbox", "When fingers lift"), value: "lift" },
                    { text: i18nc("@item:inlistbox", "Mid-swipe, after a set distance"), value: "midswipe" },
                ]
                enabled: kcm.pluginEnabled
                textRole: "text"
                model: choices
                currentIndex: (kcm.revision, kcm.value("TriggerTiming") === "midswipe" ? 1 : 0)
                onActivated: index => kcm.setValue("TriggerTiming", choices[index].value)
            }
            Kirigami.ContextualHelpButton {
                toolTipText: i18nc("@info:tooltip", "When fingers lift: the action runs when you finish the swipe, so you can still change your mind. Mid-swipe: it runs as soon as you've swiped the set distance, while your fingers are still moving, which feels faster and isn't lost if your fingers lift unevenly. The window switcher always follows your fingers either way.")
            }
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Mid-swipe distance:")
            key: "TriggerDistance"
            from: 40; to: 300; stepSize: 10
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
            active: kcm.pluginEnabled && (kcm.revision, kcm.value("TriggerTiming")) === "midswipe"
            helpText: i18nc("@info:tooltip", "How far you swipe before a mid-swipe action runs. Shorter feels snappier; longer makes accidental triggers less likely. Only used with mid-swipe actions.")
        }
        RowLayout {
            QQC2.CheckBox {
                text: i18nc("@option:check", "Swipe back to undo")
                enabled: kcm.pluginEnabled && (kcm.revision, kcm.value("TriggerTiming")) === "midswipe"
                checked: (kcm.revision, kcm.value("ReverseUndo")) === "true"
                onToggled: kcm.setValue("ReverseUndo", checked ? "true" : "false")
            }
            Kirigami.ContextualHelpButton {
                toolTipText: i18nc("@info:tooltip", "After a mid-swipe action runs, move back the other way without lifting to undo it: swipe up to open Overview, then back down to close it. Move forward again to redo. Only available with mid-swipe actions.")
            }
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Swipe starts after:")
            key: "Threshold"
            helpText: i18nc("@info:tooltip", "How far your fingers move before the plugin decides the direction (up, down, left or right). Raise it if swipes are sometimes taken the wrong way.")
            from: 10; to: 100; stepSize: 5
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Switcher moves sideways every:")
            key: "SwitcherStep"
            helpText: i18nc("@info:tooltip", "How far to slide left or right in the window switcher to move to the next window.")
            from: 40; to: 300; stepSize: 10
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Switcher moves up/down every:")
            key: "SwitcherStepVertical"
            helpText: i18nc("@info:tooltip", "How far to slide up or down in the window switcher to move to the next row (or the next window, if the switcher is a single row). Touchpads are shorter than they are wide, so this is shorter by default.")
            from: 20; to: 300; stepSize: 10
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Pinch starts after:")
            key: "PinchThreshold"
            helpText: i18nc("@info:tooltip", "How much your fingers must pinch together or spread apart before a pinch counts.")
            from: 0.05; to: 0.5; stepSize: 0.05; decimals: 2
            lowText: i18nc("@label:slider small pinch", "Small")
            highText: i18nc("@label:slider large pinch", "Large")
        }
    }
}
