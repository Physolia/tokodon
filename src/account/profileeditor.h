// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtQml>

class AbstractAccount;

/**
 * @brief Class responsible for editing the account personal information and preferences.
 */
class ProfileEditorBackend : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(AbstractAccount *account READ account WRITE setAccount NOTIFY accountChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString displayNameHtml READ displayNameHtml NOTIFY displayNameChanged)
    Q_PROPERTY(QString note READ note WRITE setNote NOTIFY noteChanged)
    Q_PROPERTY(bool bot READ bot WRITE setBot NOTIFY botChanged)
    Q_PROPERTY(QUrl backgroundUrl READ backgroundUrl WRITE setBackgroundUrl NOTIFY backgroundUrlChanged)
    Q_PROPERTY(QUrl avatarUrl READ avatarUrl WRITE setAvatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(QString backgroundUrlError READ backgroundUrlError NOTIFY backgroundUrlChanged)
    Q_PROPERTY(QString avatarUrlError READ avatarUrlError NOTIFY avatarUrlChanged)
    Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY lockedChanged)
    Q_PROPERTY(bool discoverable READ discoverable WRITE setDiscoverable NOTIFY discoverableChanged)

public:
    explicit ProfileEditorBackend(QObject *parent = nullptr);
    ~ProfileEditorBackend() override;

    /**
     * @return The account we're updating.
     */
    AbstractAccount *account() const;

    /**
     * @brief Sets the account to edit.
     * @param account The account to edit.
     */
    void setAccount(AbstractAccount *account);

    /**
     * @return The display name of the account.
     */
    QString displayName() const;

    /**
     * @return The display name of the account, processed to HTML (for custom emojis)
     */
    QString displayNameHtml() const;

    /**
     * @brief Sets a new display name for the account.
     * @param displayName The new display name, should be plain text.
     */
    void setDisplayName(const QString &displayName);

    /**
     * @return The biography text for the account.
     */
    QString note() const;

    /**
     * @brief Sets a new biography text for the account.
     * @param note The new note.
     */
    void setNote(const QString &note);

    /**
     * @return If the account should be marked as a bot.
     */
    bool bot() const;

    /**
     * @brief Sets if the account should be marked as a bot.
     * @param bot If true, the account is marked as a bot and will be visible on the profile.
     */
    void setBot(bool bot);

    /**
     * @return The current avatar URL for the account.
     */
    QUrl avatarUrl() const;

    /**
     * @return A localized error if the new avatar failed to upload.
     */
    QString avatarUrlError() const;

    /**
     * @brief Sets a new avatar URL for the account.
     * @param avatarUrl The new media URL for the avatar.
     */
    void setAvatarUrl(const QUrl &avatarUrl);

    /**
     * @return The current background URL for the account.
     */
    QUrl backgroundUrl() const;

    /**
     * @return A localized error if the new background failed to upload.
     */
    QString backgroundUrlError() const;

    /**
     * @brief Sets a new background URL for the account.
     * @param backgroundUrl The new media URL for the background.
     */
    void setBackgroundUrl(const QUrl &backgroundUrl);

    /**
     * @return Whether the account should be discoverable on the server directory.
     */
    bool discoverable() const;

    /**
     * @brief Sets whether the account should be discoverable on the server directory.
     * @param discoverable If set to true, the account will be listed on the server's directory.
     */
    void setDiscoverable(bool discoverable);

    /**
     * @return If the account is locked.
     */
    bool locked() const;

    /**
     * @brief Sets if the account is locked or not.
     * @param locked If true, the account is locked.
     */
    void setLocked(bool locked);

public Q_SLOTS:
    void save();
    void fetchAccountInfo();

Q_SIGNALS:
    void accountChanged();
    void displayNameChanged();
    void noteChanged();
    void botChanged();
    void avatarUrlChanged();
    void backgroundUrlChanged();
    void discoverableChanged();
    void lockedChanged();
    void sendNotification(const QString &message, const QString &type = QStringLiteral("info"));

private:
    AbstractAccount *m_account = nullptr;
    QString m_displayName;
    QString m_note;
    bool m_bot = false;
    QUrl m_avatarUrl;
    QUrl m_backgroundUrl;
    bool m_discoverable = true;
    bool m_locked = false;
};
