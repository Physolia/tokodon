// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2022 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigami 2.14 as Kirigami
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kmasto 1.0
import QtGraphicalEffects 1.0
import Qt.labs.qmlmodels 1.0
import QtMultimedia 5.15

QQC2.Control {
    id: root

    required property var attachments
    required property bool sensitive
    required property bool secondary
    required property bool expandedPost

    // Only uncrop timeline media if requested by the user, and there's only one attachment
    // Expanded posts (like in threads) are always uncropped.
    readonly property var shouldKeepAspectRatio: (!Config.cropMedia || root.expandedPost) && root.attachments.length === 1

    property bool isSensitive: (AccountManager.selectedAccount.preferences.extendMedia === "hide_all" ? true : (AccountManager.selectedAccount.preferences.extendMedia === "show_all" ? false : root.sensitive))

    readonly property var mediaRatio: 9.0 / 16.0

    property bool hasValidAttachment: {
        for (let i in root.attachments) {
            if (root.attachments[i].attachmentType !== Attachment.Unknown) {
                return true;
            }
        }
        return false;
    }

    Layout.fillWidth: true
    Layout.fillHeight: shouldKeepAspectRatio
    Layout.topMargin: Kirigami.Units.largeSpacing

    topPadding: 0
    leftPadding: 0
    bottomPadding: 0
    rightPadding: 0

    visible: tootContent.visible && !root.secondary && root.attachments.length > 0 && !filtered

    contentItem: GridLayout {
        id: attachmentGridLayout
        columns: root.attachments.length > 1 ? 2 : 1

        Repeater {
            id: attachmentsRepeater
            model: root.secondary ? [] : attachments

            DelegateChooser {
                role: "attachmentType"

                DelegateChoice {
                    roleValue: Attachment.Image

                    MediaContainer {
                        required property int index
                        required property var modelData

                        repeater: attachmentsRepeater
                        aspectRatio: img.sourceSize.height / img.sourceSize.width
                        shouldKeepAspectRatio: root.shouldKeepAspectRatio
                        mediaRatio: root.mediaRatio
                        rootWidth: root.width
                        gridLayout: attachmentGridLayout

                        Image {
                            id: img

                            anchors.fill: parent

                            source: modelData.previewUrl
                            mipmap: true
                            cache: true
                            fillMode: Image.PreserveAspectCrop
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Item {
                                    width: img.width
                                    height: img.height
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: img.width
                                        height: img.height
                                        radius: Kirigami.Units.smallSpacing
                                    }
                                }
                            }

                            TapHandler {
                                onTapped: {
                                    if (root.isSensitive) {
                                        root.isSensitive = false;
                                    } else {
                                        Navigation.openFullScreenImage(root.attachments, img.index);
                                    }
                                }
                            }

                            Image {
                                anchors.fill: parent
                                source: visible ? "image://blurhash/" + modelData.blurhash : ''
                                visible: parent.status !== Image.Ready || root.isSensitive
                            }

                            QQC2.Button {
                                visible: modelData.attachmentType === Attachment.Unknown
                                text: i18n("Not available")
                                anchors.centerIn: parent
                                onClicked: Qt.openUrlExternally(modelData.remoteUrl)
                            }
                        }
                    }
                }

                DelegateChoice {
                    roleValue: Attachment.GifV

                    MediaContainer {
                        required property int index
                        required property var modelData

                        repeater: attachmentsRepeater
                        aspectRatio: output.sourceRect.height / output.sourceRect.width
                        shouldKeepAspectRatio: root.shouldKeepAspectRatio
                        mediaRatio: root.mediaRatio
                        rootWidth: root.width
                        gridLayout: attachmentGridLayout

                        MediaPlayer {
                            id: player

                            autoPlay: true

                            loops: MediaPlayer.Infinite

                            source: modelData.url
                            videoOutput: output
                        }

                        VideoOutput {
                            id: output

                            source: player
                            fillMode: VideoOutput.PreserveAspectCrop
                            flushMode: VideoOutput.FirstFrame

                            anchors.fill: parent

                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Item {
                                    width: output.width
                                    height: output.height
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: output.width
                                        height: output.height
                                        radius: Kirigami.Units.smallSpacing
                                    }
                                }
                            }
                        }

                        TapHandler {
                            onTapped: {
                                if (root.isSensitive) {
                                    root.isSensitive = false;
                                } else {
                                    Navigation.openFullScreenImage(root.attachments, parent.index);
                                }
                            }
                        }

                        Image {
                            anchors.fill: parent
                            source: visible ? "image://blurhash/" + modelData.blurhash : ''
                            visible: player.status === MediaPlayer.Loading || root.isSensitive
                        }

                        QQC2.Button {
                            visible: modelData.attachmentType === Attachment.Unknown
                            text: i18n("Not available")
                            anchors.centerIn: parent
                            onClicked: Qt.openUrlExternally(modelData.remoteUrl)
                        }
                    }
                }
            }
        }
    }

    QQC2.Button {
        icon.name: "view-hidden"

        visible: !parent.isSensitive && parent.hasValidAttachment

        anchors.top: parent.top
        anchors.topMargin: Kirigami.Units.smallSpacing
        anchors.left: parent.left
        anchors.leftMargin: Kirigami.Units.smallSpacing

        onClicked: root.isSensitive = true
    }

    QQC2.Button {
        anchors.centerIn: parent

        visible: parent.isSensitive && parent.hasValidAttachment

        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
        Kirigami.Theme.inherit: false

        text: i18n("Media Hidden")
        onClicked: if (root.isSensitive) {
            root.isSensitive = false;
        } else {
            Navigation.openFullScreenImage(root.attachments, 0);
        }
    }
}
