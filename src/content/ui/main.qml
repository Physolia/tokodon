// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
// SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import org.kde.kmasto 1.0

Kirigami.ApplicationWindow {
    id: appwindow

    minimumWidth: Kirigami.Units.gridUnit * 15
    minimumHeight: Kirigami.Units.gridUnit * 20
    pageStack.defaultColumnWidth: Kirigami.Units.gridUnit * 30
    pageStack.globalToolBar.canContainHandles: true

    Connections {
        target: AccountManager
        function onAccountSelected() {
            pageStack.clear();
            pageStack.push(mainTimeline);
        }

        function onAccountRemoved() {
            if (!AccountManager.hasAccount) {
                pageStack.replace('qrc:/content/ui/LoginPage.qml');
            }
        }
    }

    globalDrawer: Kirigami.OverlayDrawer {
        id: drawer
        edge: Qt.application.layoutDirection === Qt.RightToLeft ? Qt.RightEdge : Qt.LeftEdge
        modal: Kirigami.Settings.isMobile || (applicationWindow().width < Kirigami.Units.gridUnit * 50 && !collapsed) // Only modal when not collapsed, otherwise collapsed won't show.
        drawerOpen: !Kirigami.Settings.isMobile
        width: Kirigami.Units.gridUnit * 16
        Behavior on width { NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad } }
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        
        handleClosedIcon.source: modal ? null : "sidebar-expand-left"
        handleOpenIcon.source: modal ? null : "sidebar-collapse-left"
        handleVisible: applicationWindow().pageStack.depth <= 1 && applicationWindow().pageStack.layers.depth <= 1

        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        contentItem: ColumnLayout {
            QQC2.ToolBar {
                id: toolbar
                Layout.fillWidth: true
                Layout.preferredHeight: pageStack.globalToolBar.preferredHeight

                leftPadding: Kirigami.Units.smallSpacing
                rightPadding: Kirigami.Units.smallSpacing
                topPadding: 0
                bottomPadding: 0

                RowLayout {
                    anchors.fill: parent

                    Kirigami.Heading {
                        Layout.leftMargin: Kirigami.Units.smallSpacing + Kirigami.Units.largeSpacing
                        text: i18n("Tokodon")
                    }
                }
            }

            Repeater {
                model: AccountManager
                delegate: Kirigami.BasicListItem {
                    label: model.display
                    subtitle: model.description
                    highlighted: AccountManager.selectedAccount === model.account
                    leading: Kirigami.Avatar {
                        source: model.account.identity.avatarUrl
                        name: model.display
                        implicitWidth: height
                        sourceSize.width: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
                        sourceSize.height: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
                    }
                    onClicked: if (AccountManager.selectedAccount !== model.account) {
                        AccountManager.selectedAccount = model.account;
                    }
                    trailing: QQC2.ToolButton {
			icon.name: 'im-user'
                        onClicked: applicationWindow().pageStack.pushDialogLayer('qrc:/content/ui/ProfileEditor.qml', {account: model.account}, {title: i18n('Profile Editor')})
                    }
                }
            }
            Kirigami.BasicListItem {
                action: Kirigami.Action {
                    icon.name: "list-add"
                    onTriggered: pageStack.layers.push('qrc:/content/ui/LoginPage.qml');
                    text: i18n("Add Account")
                }
                enabled: AccountManager.hasAccounts && pageStack.get(0).objectName !== 'loginPage' && pageStack.get(0).objectName !== 'authorizationPage' && (pageStack.layers.depth === 1 || pageStack.layers.get(1).objectName !== 'loginPage' && pageStack.layers.get(1).objectName !== 'authorizationPage')
            }
            Item {
                Layout.fillHeight: true
            }
            Kirigami.BasicListItem {
                action: Kirigami.Action {
                    icon.name: "settings-configure"
                    onTriggered: pageStack.pushDialogLayer('qrc:/content/ui/Settings/SettingsPage.qml');
                    text: i18nc("@action:inmenu", "Settings")
                }
            }
        }
    }

    footer: Kirigami.NavigationTabBar {
        // Make sure we take in count drawer width
        anchors.left: parent.left
        anchors.right: parent.right
        visible: pageStack.layers.depth <= 1 && AccountManager.hasAccounts
        actions: [
            Kirigami.Action {
                iconName: "go-home-large"
                text: i18n("Home")
                checked: pageStack.currentItem.title === i18n("Home")
                onTriggered: {
                    pageStack.layers.clear();
                    pageStack.replace(mainTimeline);
                }
            },
            Kirigami.Action {
                iconName: "notifications"
                text: i18n("Notifications")
                checked: pageStack.currentItem.title === i18n("Home")
                onTriggered: appwindow.showPassiveNotification(i18n("Notifications support is not implemented yet"));
            },
            Kirigami.Action {
                iconName: "system-users"
                text: i18n("Local")
                checked: pageStack.currentItem.title === i18n("Local Timeline")
                onTriggered: {
                    pageStack.layers.clear();
                    pageStack.replace(mainTimeline, { name: "public" });
                }
            },
            Kirigami.Action {
                iconName: "kstars_xplanet"
                text: i18n("Global")
                checked: pageStack.currentItem.title === i18n("Global Timeline")
                onTriggered: {
                    pageStack.layers.clear();
                    pageStack.replace(mainTimeline, { name: "federated" });
                }
            }
        ]
    }
    //header: Kirigami.Settings.isMobile ? null : toolBar

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    Component.onCompleted: {
        if (AccountManager.hasAccounts) {
            pageStack.push(mainTimeline);
        } else {
            pageStack.push('qrc:/content/ui/LoginPage.qml');
        }
    }


    Component {
        id: mainTimeline
        TimelinePage {
            property alias name: timelineModel.name
            model: TimelineModel {
                id: timelineModel
                accountManager: AccountManager
                name: "home"
            }
        }
    }
}
