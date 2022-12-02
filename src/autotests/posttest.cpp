// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QtTest>

#include "mockaccount.h"
#include "post.h"
#include <QJsonObject>

class PostTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
    }

    void testFromJson()
    {
        MockAccount account;

        QFile statusExampleApi;
        statusExampleApi.setFileName(QLatin1String(DATA_DIR) + QLatin1Char('/') + "status.json");
        statusExampleApi.open(QIODevice::ReadOnly);

        const auto doc = QJsonDocument::fromJson(statusExampleApi.readAll());
        Post post(&account, doc.object());

        QCOMPARE(post.subject(), "SPOILER");
        QCOMPARE(post.content(), "<p>LOREM</p>");
        QVERIFY(post.card());
        QCOMPARE(post.contentType(), QString());
        QCOMPARE(post.isSensitive(), false);
        QCOMPARE(post.visibility(), Post::Visibility::Public);

        QCOMPARE(post.authorIdentity()->displayName(), "Eugen :kde:");
        QCOMPARE(post.authorIdentity()->displayNameHtml(), "Eugen <img height=\"16\" width=\"16\" src=\"https://kde.org\">");
    }
};

QTEST_MAIN(PostTest)
#include "posttest.moc"