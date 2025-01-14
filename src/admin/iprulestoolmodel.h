// SPDX-FileCopyrightText: 2023 Rishi Kumar <rsi.dev17@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#pragma once

#include "ipinfo.h"

#include <QtQml>

// todo: use std::chrono when c++20 is stable
#define YEAR 31536000
#define DAY 86400

class IpRulesToolModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    enum CustomRoles {
        IdRole,
        IpRole,
        SeverityRole,
        CommentRole,
        CreatedAtRole,
        ExpiredAtRole,
    };

    enum TimeInterval {
        ThreeYears = YEAR * 3,
        OneYear = YEAR,
        Sixmonths = YEAR / 2,
        Onemonth = YEAR / 12,
        Twoweeks = DAY * 14,
        Oneday = DAY,
    };

    Q_ENUM(TimeInterval)

    explicit IpRulesToolModel(QObject *parent = nullptr);

    bool loading() const;
    void setLoading(bool loading);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

    void filltimeline();

    Q_INVOKABLE void newIpBlock(const QString &ip, int expiresIn, const QString &comment, const QString &severity);
    Q_INVOKABLE void deleteIpBlock(int row);
    Q_INVOKABLE void updateIpBlock(int row, const QString &ip, const QString &severity, const QString &comment, int expiresIn);

Q_SIGNALS:
    void loadingChanged();

private:
    QList<IpInfo> m_ipinfo;
    bool m_loading = false;
    QUrl m_next;
};
