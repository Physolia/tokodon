// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls 2 as QQC2
import QtQuick.Layouts
import Qt.labs.qmlmodels 1.0
import org.kde.kirigami 2 as Kirigami
import org.kde.tokodon
import org.kde.tokodon.private
import "./StatusDelegate"
import "./Components"

QQC2.ItemDelegate {
    id: root

    required property int index
    required property var notificationActorIdentity
    required property bool selected

    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.largeSpacing * 2
    leftPadding: Kirigami.Units.largeSpacing * 2
    rightPadding: Kirigami.Units.largeSpacing * 2

    highlighted: false
    hoverEnabled: false

    width: ListView.view.width

    background: null

    contentItem: Kirigami.FlexColumn {
        maximumWidth: Kirigami.Units.gridUnit * 40
        spacing: 0

        RowLayout {
            Layout.topMargin: visible ? Kirigami.Units.smallSpacing : 0
            Layout.bottomMargin: visible ? Kirigami.Units.smallSpacing : 0
            Kirigami.Icon {
                source: 'list-add-user'
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                color: Kirigami.Theme.disabledTextColor
                Layout.preferredHeight: Kirigami.Units.largeSpacing * 2
                Layout.preferredWidth: Kirigami.Units.largeSpacing * 2
            }

            QQC2.Label {
                font: Config.defaultFont
                text: i18n("%1 followed you", root.notificationActorIdentity.displayNameHtml)
                textFormat: Text.RichText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }

        UserCard {
            userIdentity: root.notificationActorIdentity
        }
    }
}