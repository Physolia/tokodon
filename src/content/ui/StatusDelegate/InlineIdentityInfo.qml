// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import org.kde.kirigami 2 as Kirigami
import QtQuick.Controls 2 as QQC2
import QtQuick.Layouts
import org.kde.kirigamiaddons.components 1 as KirigamiComponents

import org.kde.tokodon
import org.kde.tokodon.private

// Used everywhere, is a component with an Avatar, some labels and such smashed together
RowLayout {
    id: root

    required property var identity
    required property bool secondary
    property bool admin: false
    property string ip
    readonly property alias avatar: avatar

    spacing: Kirigami.Units.mediumSpacing

    signal clicked()

    KirigamiComponents.AvatarButton {
        id: avatar

        Layout.alignment: admin ? Qt.AlignCenter : Qt.AlignTop
        Layout.rowSpan: 5

        source: root.identity.avatarUrl
        cache: true
        text: i18n("View Profile")
        onClicked: if (!admin) {
            Navigation.openAccount(root.identity.id);
            root.clicked();
        }
        name: root.identity.displayName
    }

    ColumnLayout {
        id: layout

        Layout.fillWidth: true

        spacing: 0
        clip: true

        Kirigami.Heading {
            level: 4
            font.pixelSize: Config.defaultFont.pixelSize + 2
            font.pointSize: -1
            text: root.identity.displayNameHtml
            type: Kirigami.Heading.Type.Primary
            color: root.secondary ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
            verticalAlignment: Text.AlignTop
            elide: Text.ElideRight
            textFormat: Text.RichText

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: avatar.clicked()
            }
        }

        QQC2.Label {
            font.pixelSize: Config.defaultFont.pixelSize + 1
            elide: Text.ElideRight
            color: Kirigami.Theme.disabledTextColor
            text: `@${root.identity.account}`
            verticalAlignment: Text.AlignTop

            width: parent.width

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: avatar.clicked()
            }
        }
        Kirigami.Heading {
            id: emailHeading
            level: 4
            text: root.ip ? root.ip : ""
            visible: admin && root.ip
            type: Kirigami.Heading.Type.Secondary
            verticalAlignment: Text.AlignTop
            elide: Text.ElideRight
        }
    }
}
