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

        enabled: kcm.pluginEnabled

        QQC2.Label { text: tuning.lowText }
        QQC2.Slider {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 12
            from: tuning.from
            to: tuning.to
            stepSize: tuning.stepSize
            value: (kcm.revision, Number(kcm.value(tuning.key)))
            onMoved: kcm.setValue(tuning.key, value.toFixed(tuning.decimals))
        }
        QQC2.Label { text: tuning.highText }
    }

    header: Kirigami.InlineMessage {
        visible: !kcm.pluginEnabled
        type: Kirigami.MessageType.Information
        position: Kirigami.InlineMessage.Position.Header
        text: i18nc("@info", "Gestures are off. Plasma's own touchpad gestures are used instead.")
    }

    Kirigami.FormLayout {
        QQC2.Switch {
            Kirigami.FormData.label: i18nc("@label", "Touchpad gestures:")
            text: i18nc("@option:check", "Enabled")
            checked: kcm.pluginEnabled
            onToggled: kcm.pluginEnabled = checked
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Three-finger swipe")
        }
        GestureRow { key: "Swipe3Up"; Kirigami.FormData.label: i18nc("@label", "Up:") }
        GestureRow { key: "Swipe3Down"; Kirigami.FormData.label: i18nc("@label", "Down:") }
        GestureRow { key: "Swipe3Left"; Kirigami.FormData.label: i18nc("@label", "Left:") }
        GestureRow { key: "Swipe3Right"; Kirigami.FormData.label: i18nc("@label", "Right:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Four-finger swipe")
        }
        GestureRow { key: "Swipe4Up"; Kirigami.FormData.label: i18nc("@label", "Up:") }
        GestureRow { key: "Swipe4Down"; Kirigami.FormData.label: i18nc("@label", "Down:") }
        GestureRow { key: "Swipe4Left"; Kirigami.FormData.label: i18nc("@label", "Left:") }
        GestureRow { key: "Swipe4Right"; Kirigami.FormData.label: i18nc("@label", "Right:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "While Overview is open")
        }
        GestureRow { key: "Swipe3Up.Overview"; Kirigami.FormData.label: i18nc("@label", "Three fingers up:") }
        GestureRow { key: "Swipe3Down.Overview"; Kirigami.FormData.label: i18nc("@label", "Three fingers down:") }
        GestureRow { key: "Swipe4Up.Overview"; Kirigami.FormData.label: i18nc("@label", "Four fingers up:") }
        GestureRow { key: "Swipe4Down.Overview"; Kirigami.FormData.label: i18nc("@label", "Four fingers down:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "While the desktop is shown")
        }
        GestureRow { key: "Swipe3Up.Desktop"; Kirigami.FormData.label: i18nc("@label", "Three fingers up:") }
        GestureRow { key: "Swipe3Down.Desktop"; Kirigami.FormData.label: i18nc("@label", "Three fingers down:") }
        GestureRow { key: "Swipe4Up.Desktop"; Kirigami.FormData.label: i18nc("@label", "Four fingers up:") }
        GestureRow { key: "Swipe4Down.Desktop"; Kirigami.FormData.label: i18nc("@label", "Four fingers down:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Pinch")
        }
        GestureRow { key: "Pinch2In"; Kirigami.FormData.label: i18nc("@label", "Two fingers in:") }
        GestureRow { key: "Pinch2Out"; Kirigami.FormData.label: i18nc("@label", "Two fingers out:") }
        QQC2.Label {
            enabled: kcm.pluginEnabled
            Layout.preferredWidth: Kirigami.Units.gridUnit * 16
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            text: i18nc("@info", "Binding a two-finger pinch stops apps from using it to zoom.")
        }
        GestureRow { key: "Pinch3In"; Kirigami.FormData.label: i18nc("@label", "Three fingers in:") }
        GestureRow { key: "Pinch3Out"; Kirigami.FormData.label: i18nc("@label", "Three fingers out:") }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Sensitivity")
        }
        QQC2.ComboBox {
            readonly property var choices: [
                { text: i18nc("@item:inlistbox", "When fingers lift"), value: "lift" },
                { text: i18nc("@item:inlistbox", "Mid-swipe, after a set distance"), value: "midswipe" },
            ]
            Kirigami.FormData.label: i18nc("@label:listbox", "Run actions:")
            enabled: kcm.pluginEnabled
            textRole: "text"
            model: choices
            currentIndex: (kcm.revision, kcm.value("TriggerTiming") === "midswipe" ? 1 : 0)
            onActivated: index => kcm.setValue("TriggerTiming", choices[index].value)
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Mid-swipe distance:")
            key: "TriggerDistance"
            from: 40; to: 300; stepSize: 10
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
            enabled: kcm.pluginEnabled && (kcm.revision, kcm.value("TriggerTiming")) === "midswipe"
        }
        QQC2.CheckBox {
            text: i18nc("@option:check", "Swipe back to undo")
            enabled: kcm.pluginEnabled && (kcm.revision, kcm.value("TriggerTiming")) === "midswipe"
            checked: (kcm.revision, kcm.value("ReverseUndo")) === "true"
            onToggled: kcm.setValue("ReverseUndo", checked ? "true" : "false")
        }
        QQC2.Label {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 16
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            enabled: kcm.pluginEnabled
            text: i18nc("@info", "Mid-swipe runs the action while your fingers are still moving. With Swipe back to undo, moving back the other way before lifting undoes it, e.g. closes the Overview you just opened. The window switcher always follows your fingers.")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Swipe starts after:")
            key: "Threshold"
            from: 10; to: 100; stepSize: 5
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Switcher moves sideways every:")
            key: "SwitcherStep"
            from: 40; to: 300; stepSize: 10
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Switcher moves up/down every:")
            key: "SwitcherStepVertical"
            from: 20; to: 300; stepSize: 10
            lowText: i18nc("@label:slider short distance", "Short")
            highText: i18nc("@label:slider long distance", "Long")
        }
        TuningSlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Pinch starts after:")
            key: "PinchThreshold"
            from: 0.05; to: 0.5; stepSize: 0.05; decimals: 2
            lowText: i18nc("@label:slider small pinch", "Small")
            highText: i18nc("@label:slider large pinch", "Large")
        }
    }
}
