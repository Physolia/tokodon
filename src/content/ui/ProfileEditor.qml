// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kmasto 1.0

MastoPage {
    property var account 

    property Identity identity: Identity {
        id: newIdentity
    }

    title: i18n('Profile Editor')

    Kirigami.FormLayout {
        QQC2.TextField {
            Kirigami.FormData.label: i18n("Display Name:") 
            text: account.identity.displayName
            onTextChanged: newIdentity.displayName = text
        }

        QQC2.TextArea {
            id: bioField
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            Kirigami.FormData.label: i18n("Bio:") 
            text: account.identity.bio
            textFormat: TextEdit.RichText
        }

        QQC2.CheckBox {
            text: i18n("Require follow requests")
            checked: account.identity.locked
	}

        QQC2.CheckBox {
            text: i18n("This is a bot account")
            checked: account.identity.bot
	}

        QQC2.CheckBox {
            text: i18n("Suggest account to others")
            checked: account.identity.discoverable
	}
    }

    footer: RowLayout {
        Item {
            Layout.fillWidth: true
        }

        QQC2.Button {
            text: i18n('Save')
            icon.name: 'dialog-ok'
            Layout.margins: Kirigami.Units.smallSpacing
            onClicked: account.saveAccount(newIdentity)
        }
    }
}
