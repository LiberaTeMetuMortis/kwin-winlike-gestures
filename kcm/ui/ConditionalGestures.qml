/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

// Sub-page: one card per situation (state suffix), listing the gestures that
// do something else there. Gestures without a rule keep their normal action.
KCM.SimpleKCM {
    id: page

    title: i18nc("@title", "Conditional Gestures")

    // Situations added with "Add New" that have no rule yet. Only kept while
    // the page is open: an empty situation isn't saved.
    property var openedEmpty: []

    readonly property var situationInfo: ({
        ".Grid": { title: i18nc("@title:group", "While the desktop grid is open"), icon: "view-grid-symbolic" },
        ".Overview": { title: i18nc("@title:group", "While Overview is open"), icon: "view-app-grid-symbolic" },
        ".Desktop": { title: i18nc("@title:group", "While the desktop is shown"), icon: "user-desktop-symbolic" },
    })

    function rulesOf(suffix) {
        return (kcm.revision, kcm.rules(suffix));
    }

    function isShown(suffix) {
        return rulesOf(suffix).length > 0 || openedEmpty.includes(suffix);
    }

    function gestureName(key) {
        const m = key.match(/^(Swipe|Pinch)(\d)(Up|Down|Left|Right|In|Out)/);
        if (!m) {
            return key;
        }
        const n = Number(m[2]);
        switch (m[1] + m[3]) {
        case "SwipeUp": return i18ncp("@label", "%1 finger up", "%1 fingers up", n);
        case "SwipeDown": return i18ncp("@label", "%1 finger down", "%1 fingers down", n);
        case "SwipeLeft": return i18ncp("@label", "%1 finger left", "%1 fingers left", n);
        case "SwipeRight": return i18ncp("@label", "%1 finger right", "%1 fingers right", n);
        case "PinchIn": return i18ncp("@label", "%1-finger pinch in", "%1-finger pinch in", n);
        case "PinchOut": return i18ncp("@label", "%1-finger pinch out", "%1-finger pinch out", n);
        }
        return key;
    }

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Add New")
            icon.name: "list-add"
            Kirigami.Action {
                text: page.situationInfo[".Grid"].title
                icon.name: page.situationInfo[".Grid"].icon
                enabled: !page.isShown(".Grid")
                onTriggered: page.openedEmpty = page.openedEmpty.concat([".Grid"])
            }
            Kirigami.Action {
                text: page.situationInfo[".Overview"].title
                icon.name: page.situationInfo[".Overview"].icon
                enabled: !page.isShown(".Overview")
                onTriggered: page.openedEmpty = page.openedEmpty.concat([".Overview"])
            }
            Kirigami.Action {
                text: page.situationInfo[".Desktop"].title
                icon.name: page.situationInfo[".Desktop"].icon
                enabled: !page.isShown(".Desktop")
                onTriggered: page.openedEmpty = page.openedEmpty.concat([".Desktop"])
            }
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit * 4
            visible: !kcm.situations().some(s => page.isShown(s))
            icon.name: "input-touchpad"
            text: i18nc("@info:placeholder", "No special situations")
            explanation: xi18nc("@info:placeholder", "Click <interface>Add New</interface> to make gestures do something else while Overview or the desktop grid is open, or while the desktop is shown.")
        }

        Repeater {
            model: kcm.situations()

            delegate: Kirigami.Card {
                id: card

                required property string modelData
                readonly property string suffix: modelData
                readonly property var rules: page.rulesOf(suffix)
                readonly property var missing: (kcm.revision, kcm.gestureKeys().filter(g => !card.rules.includes(g + card.suffix)))

                Layout.fillWidth: true
                visible: page.isShown(suffix)

                banner.title: page.situationInfo[suffix].title
                banner.titleIcon: page.situationInfo[suffix].icon
                banner.titleLevel: 3

                actions: [
                    Kirigami.Action {
                        text: i18nc("@action:button", "Add Gesture…")
                        icon.name: "list-add"
                        enabled: kcm.pluginEnabled && card.missing.length > 0
                        onTriggered: addMenu.popup()
                    },
                    Kirigami.Action {
                        text: i18nc("@action:button", "Remove Situation")
                        icon.name: "edit-delete"
                        enabled: kcm.pluginEnabled
                        onTriggered: {
                            kcm.removeSituation(card.suffix);
                            page.openedEmpty = page.openedEmpty.filter(s => s !== card.suffix);
                        }
                    }
                ]

                QQC2.Menu {
                    id: addMenu
                    Instantiator {
                        model: card.missing
                        delegate: QQC2.MenuItem {
                            required property string modelData
                            text: page.gestureName(modelData)
                            onTriggered: kcm.addRule(modelData + card.suffix)
                        }
                        onObjectAdded: (index, object) => addMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => addMenu.removeItem(object)
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        visible: card.rules.length === 0
                        text: i18nc("@info", "No gestures yet. Use Add Gesture to choose one.")
                        color: Kirigami.Theme.disabledTextColor
                    }

                    Repeater {
                        model: card.rules
                        delegate: RowLayout {
                            required property string modelData
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label {
                                text: page.gestureName(modelData) + ":"
                                Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                                elide: Text.ElideRight
                            }
                            GestureRow {
                                key: modelData
                            }
                            // Keeps the delete buttons in one column.
                            Item {
                                Layout.fillWidth: true
                            }
                            QQC2.ToolButton {
                                icon.name: "edit-delete"
                                enabled: kcm.pluginEnabled
                                display: QQC2.AbstractButton.IconOnly
                                text: i18nc("@action:button", "Remove this gesture")
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                onClicked: {
                                    // Keep the card when its last rule goes, so another
                                    // gesture can be added without Add New again.
                                    if (card.rules.length === 1 && !page.openedEmpty.includes(card.suffix)) {
                                        page.openedEmpty = page.openedEmpty.concat([card.suffix]);
                                    }
                                    kcm.removeRule(modelData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
