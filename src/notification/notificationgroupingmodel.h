// SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>
// SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-FileCopyrightText: 2023 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include "notificationmodel.h"

class NotificationGroupingModel : public QAbstractProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(NotificationModel *sourceModel WRITE setSourceModel READ getSourceModel NOTIFY sourceModelChanged)

public:
    explicit NotificationGroupingModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;
    bool hasChildren(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &proxyIndex, int role) const override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    bool loading() const;

    void setSourceModel(NotificationModel *model);
    NotificationModel *getSourceModel();

Q_SIGNALS:
    void loadingChanged();
    void sourceModelChanged();

private:
    bool notificationsMatch(const QModelIndex &a, const QModelIndex &b);
    void rebuildMap();
    void checkGrouping(bool silent = false);
    bool isGroup(int row) const;
    bool tryToGroup(const QModelIndex &sourceIndex, bool silent = false);
    void adjustMap(int anchor, int delta);

    QList<QList<int> *> rowMap;
};
