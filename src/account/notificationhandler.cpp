// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationhandler.h"

#include "account.h"
#include "networkcontroller.h"

#include <QPainter>

#include <KLocalizedString>
#include <KNotification>

NotificationHandler::NotificationHandler(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , m_nam(nam)
{
}

void NotificationHandler::handle(std::shared_ptr<Notification> notification, AbstractAccount *account)
{
    KNotification *knotification;

    const auto addViewPostAction = [this, &knotification, notification] {
        auto viewPostAction = knotification->addAction(i18n("View Post"));
        auto defaultAction = knotification->addDefaultAction(i18n("View Post"));

        auto openPost = [=] {
            NetworkController::instance().openWebApLink(notification->post()->url().toString());
        };

        connect(viewPostAction, &KNotificationAction::activated, this, openPost);
        connect(defaultAction, &KNotificationAction::activated, this, openPost);
    };

    const auto addViewUserAction = [this, &knotification, notification] {
        auto viewProfileActions = knotification->addAction(i18n("View Profile"));
        auto defaultAction = knotification->addDefaultAction(i18n("View Profile"));

        auto viewProfile = [=] {
            NetworkController::instance().openWebApLink(notification->identity()->url().toString());
        };

        connect(viewProfileActions, &KNotificationAction::activated, this, viewProfile);
        connect(defaultAction, &KNotificationAction::activated, this, viewProfile);
    };

    switch (notification->type()) {
    case Notification::Mention:
        if (!account->config()->notifyMention()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("mention"));
        knotification->setTitle(i18n("%1 mentioned you", notification->identity()->displayName()));
        addViewPostAction();
        break;
    case Notification::Status:
        if (!account->config()->notifyStatus()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("status"));
        knotification->setTitle(i18n("%1 wrote a new post", notification->identity()->displayName()));
        addViewPostAction();
        break;
    case Notification::Repeat:
        if (!account->config()->notifyBoost()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("boost"));
        knotification->setTitle(i18n("%1 boosted your post", notification->identity()->displayName()));
        addViewPostAction();
        break;
    case Notification::Follow:
        if (!account->config()->notifyFollow()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("follow"));
        knotification->setTitle(i18n("%1 followed you", notification->identity()->displayName()));
        addViewUserAction();
        break;
    case Notification::FollowRequest:
        if (!account->config()->notifyFollowRequest()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("follow-request"));
        knotification->setTitle(i18n("%1 requested to follow you", notification->identity()->displayName()));
        addViewUserAction();
        break;
    case Notification::Favorite:
        if (!account->config()->notifyFavorite()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("favorite"));
        knotification->setTitle(i18n("%1 favorited your post", notification->identity()->displayName()));
        addViewPostAction();
        break;
    case Notification::Poll:
        if (!account->config()->notifyPoll()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("poll"));
        knotification->setTitle(i18n("Poll by %1 has ended", notification->identity()->displayName()));
        addViewPostAction();
        break;
    case Notification::Update:
        if (!account->config()->notifyUpdate()) {
            return;
        }
        knotification = new KNotification(QStringLiteral("update"));
        knotification->setTitle(i18n("%1 edited a post", notification->identity()->displayName()));
        addViewPostAction();
        break;

    default:
        Q_UNREACHABLE();
    }

    if (notification->post() != nullptr) {
        knotification->setText(notification->post()->content());
    }
    knotification->setHint(QStringLiteral("x-kde-origin-name"), account->identity()->displayName());

    if (!notification->identity()->avatarUrl().isEmpty()) {
        const auto avatarUrl = notification->identity()->avatarUrl();
        auto request = QNetworkRequest(avatarUrl);
        auto reply = m_nam->get(request);
        connect(reply, &QNetworkReply::finished, this, [reply, knotification]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                knotification->sendEvent();
                return;
            }
            QPixmap img;
            img.loadFromData(reply->readAll());

            // Handle avatars that are lopsided in one dimension
            const int biggestDimension = std::max(img.width(), img.height());
            const QRect imageRect{0, 0, biggestDimension, biggestDimension};

            QImage roundedImage(imageRect.size(), QImage::Format_ARGB32);
            roundedImage.fill(Qt::transparent);

            QPainter painter(&roundedImage);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.setPen(Qt::NoPen);

            // Fill background for transparent avatars
            painter.setBrush(Qt::white);
            painter.drawRoundedRect(imageRect, imageRect.width(), imageRect.height());

            QBrush brush(img.scaledToHeight(biggestDimension));
            painter.setBrush(brush);
            painter.drawRoundedRect(imageRect, imageRect.width(),
                                    imageRect.height());
            painter.end();

            knotification->setPixmap(
                QPixmap::fromImage(std::move(roundedImage)));
            knotification->sendEvent();
        });
    } else {
        knotification->sendEvent();
    }
}

#include "moc_notificationhandler.cpp"