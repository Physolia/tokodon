// SPDX-FileCopyrightText: 2021 kaniini <https://git.pleroma.social/kaniini>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-3.0-only

#include "abstractaccount.h"

#include "accountmanager.h"
#include "messagefiltercontainer.h"
#include "navigation.h"
#include "networkcontroller.h"
#include "relationship.h"
#include "tokodon_debug.h"

#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

AbstractAccount::AbstractAccount(QObject *parent, const QString &instanceUri)
    : QObject(parent)
    , m_instance_uri(instanceUri)
    // default to 500, instances which support more signal it
    , m_maxPostLength(500)
    , m_maxPollOptions(4)
    , m_supportsLocalVisibility(false)
    , m_charactersReservedPerUrl(23)
    , m_identity(std::make_shared<Identity>())
    , m_allowedContentTypes(AllowedContentType::PlainText)
{
}

AbstractAccount::AbstractAccount(QObject *parent)
    : QObject(parent)
    // default to 500, instances which support more signal it
    , m_maxPostLength(500)
    , m_maxPollOptions(4)
    , m_supportsLocalVisibility(false)
    , m_charactersReservedPerUrl(23)
    , m_identity(std::make_shared<Identity>())
    , m_allowedContentTypes(AllowedContentType::PlainText)
{
}

AccountConfig *AbstractAccount::config()
{
    if (!m_config) {
        m_config = new AccountConfig{settingsGroupName(), this};
    }
    Q_ASSERT(m_config != nullptr);
    return m_config;
}

Preferences *AbstractAccount::preferences() const
{
    return m_preferences;
}

QString AbstractAccount::username() const
{
    return m_name;
}

void AbstractAccount::setUsername(const QString &username)
{
    if (m_name == username) {
        return;
    }
    m_name = username;
    Q_EMIT usernameChanged();
}

size_t AbstractAccount::maxPostLength() const
{
    return m_maxPostLength;
}

size_t AbstractAccount::maxPollOptions() const
{
    return m_maxPollOptions;
}

bool AbstractAccount::supportsLocalVisibility() const
{
    return m_supportsLocalVisibility;
}

size_t AbstractAccount::charactersReservedPerUrl() const
{
    return m_charactersReservedPerUrl;
}

QString AbstractAccount::instanceName() const
{
    return m_instance_name;
}

bool AbstractAccount::haveToken() const
{
    return !m_token.isEmpty();
}

bool AbstractAccount::hasName() const
{
    return !m_name.isEmpty();
}

bool AbstractAccount::hasInstanceUrl() const
{
    return !m_instance_uri.isEmpty();
}

QUrl AbstractAccount::apiUrl(const QString &path) const
{
    QUrl url = QUrl::fromUserInput(m_instance_uri);
    url.setScheme(QStringLiteral("https"));
    url.setPath(path);

    return url;
}

void AbstractAccount::registerApplication(const QString &appName, const QString &website, const QString &additionalScopes)
{
    // clear any previous bearer token credentials
    m_token = QString();

    QString ourAdditionalScopes;
#ifdef HAVE_KUNIFIEDPUSH
    ourAdditionalScopes = QStringLiteral("push ");
#endif

    // Store for future usage (e.g. authorizeUrl)
    m_additionalScopes = ourAdditionalScopes + additionalScopes;

    // register
    const QUrl regUrl = apiUrl(QStringLiteral("/api/v1/apps"));
    const QJsonObject obj{
        {QStringLiteral("client_name"), appName},
        {QStringLiteral("redirect_uris"), QStringLiteral("tokodon://oauth")},
        {QStringLiteral("scopes"), QStringLiteral("read write follow %1").arg(m_additionalScopes)},
        {QStringLiteral("website"), website},
    };
    const QJsonDocument doc(obj);

    post(regUrl, doc, false, this, [=](QNetworkReply *reply) {
        if (!reply->isFinished()) {
            qCDebug(TOKODON_LOG) << "not finished";
            return;
        }

        const auto data = reply->readAll();
        const auto doc = QJsonDocument::fromJson(data);

        m_client_id = doc.object()["client_id"_L1].toString();
        m_client_secret = doc.object()["client_secret"_L1].toString();

        s_messageFilter->insert(m_client_secret, QStringLiteral("CLIENT_SECRET"));

        if (isRegistered()) {
            Q_EMIT registered();
            fetchInstanceMetadata();
        }
    });
}

void AbstractAccount::registerAccount(const QString &username,
                                      const QString &email,
                                      const QString &password,
                                      bool agreement,
                                      const QString &locale,
                                      const QString &reason)
{
    // get an app-level access token, obviously we don't have a user token yet.
    const QUrl tokenUrl = getTokenUrl();
    QUrlQuery q = buildOAuthQuery();

    q.addQueryItem(QStringLiteral("client_secret"), m_client_secret);
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    q.addQueryItem(QStringLiteral("scope"), QStringLiteral("write"));

    post(tokenUrl, q, false, this, [=](QNetworkReply *reply) {
        auto data = reply->readAll();
        auto doc = QJsonDocument::fromJson(data);

        // override the token for now
        m_token = doc.object()["access_token"_L1].toString();
        s_messageFilter->insert(m_token, QStringLiteral("ACCESS_TOKEN"));

        const QUrlQuery formdata{
            {QStringLiteral("username"), username},
            {QStringLiteral("email"), email},
            {QStringLiteral("password"), password},
            {QStringLiteral("agreement"), agreement ? QStringLiteral("1") : QStringLiteral("0")},
            {QStringLiteral("locale"), locale},
            {QStringLiteral("reason"), reason},
        };

        post(
            apiUrl(QStringLiteral("/api/v1/accounts")),
            formdata,
            true,
            this,
            [=](QNetworkReply *reply) {
                const auto data = reply->readAll();
                const auto doc = QJsonDocument::fromJson(data);

                if (doc.object().contains("access_token"_L1)) {
                    setUsername(username);
                    setAccessToken(doc["access_token"_L1].toString());
                }
            },
            [=](QNetworkReply *reply) {
                Q_EMIT registrationError(QString::fromUtf8(reply->readAll()));
            });
    });
}

Identity *AbstractAccount::identity()
{
    return m_identity.get();
}

std::shared_ptr<Identity> AbstractAccount::identityLookup(const QString &accountId, const QJsonObject &doc)
{
    if (m_identity && m_identity->id() == accountId) {
        return m_identity;
    }
    auto id = m_identityCache[accountId];
    if (id && id->id() == accountId) {
        return id;
    }

    id = std::make_shared<Identity>();
    id->reparentIdentity(this);
    id->fromSourceData(doc);
    m_identityCache[accountId] = id;

    return m_identityCache[accountId];
}

std::shared_ptr<AdminAccountInfo> AbstractAccount::adminIdentityLookup(const QString &accountId, const QJsonObject &doc)
{
    if (m_adminIdentity && m_adminIdentity->userLevelIdentity()->id() == accountId) {
        return m_adminIdentity;
    }
    auto id = m_adminIdentityCache[accountId];
    if (id && id->userLevelIdentity()->id() == accountId) {
        return id;
    }

    id = std::make_shared<AdminAccountInfo>();
    id->reparentAdminAccountInfo(this);
    id->fromSourceData(doc);
    m_adminIdentityCache[accountId] = id;

    return m_adminIdentityCache[accountId];
}

AdminAccountInfo *AbstractAccount::adminIdentityLookupWithVanillaPointer(const QString &accountId, const QJsonObject &doc)
{
    const auto id = new AdminAccountInfo();

    id->reparentAdminAccountInfo(this);
    id->fromSourceData(doc);
    m_adminIdentityCacheWithVanillaPointer[accountId] = id;

    return m_adminIdentityCacheWithVanillaPointer[accountId];
}

std::shared_ptr<ReportInfo> AbstractAccount::reportInfoLookup(const QString &reportId, const QJsonObject &doc)
{
    if (m_reportInfo && m_reportInfo->reportId() == reportId) {
        return m_reportInfo;
    }
    auto id = m_reportInfoCache[reportId];
    if (id && id->reportId() == reportId) {
        return id;
    }

    id = std::make_shared<ReportInfo>();
    id->reparentReportInfo(this);
    id->fromSourceData(doc);
    m_reportInfoCache[reportId] = id;

    return m_reportInfoCache[reportId];
}

bool AbstractAccount::identityCached(const QString &accountId) const
{
    if (m_identity && m_identity->id() == accountId) {
        return true;
    }
    auto id = m_identityCache[accountId];
    return id && id->id() == accountId;
}

QUrlQuery AbstractAccount::buildOAuthQuery() const
{
    return QUrlQuery{{QStringLiteral("client_id"), m_client_id}};
}

QUrl AbstractAccount::getAuthorizeUrl() const
{
    QUrl url = apiUrl(QStringLiteral("/oauth/authorize"));
    QUrlQuery q = buildOAuthQuery();

    q.addQueryItem(QStringLiteral("redirect_uri"), QStringLiteral("tokodon://oauth"));
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    q.addQueryItem(QStringLiteral("scope"), QStringLiteral("read write follow ") + m_additionalScopes);

    url.setQuery(q);

    return url;
}

void AbstractAccount::setAccessToken(const QString &token)
{
    m_token = token;
    s_messageFilter->insert(m_token, QStringLiteral("ACCESS_TOKEN"));
    AccountManager::instance().addAccount(this, false);
    AccountManager::instance().selectAccount(this, true);
    validateToken(true);
}

QUrl AbstractAccount::getTokenUrl() const
{
    return apiUrl(QStringLiteral("/oauth/token"));
}

void AbstractAccount::setInstanceUri(const QString &instance_uri)
{
    // instance URI changed, get new credentials
    QUrl instance_url = QUrl::fromUserInput(instance_uri);
    instance_url.setScheme(QStringLiteral("https")); // getting token from http is not supported

    m_instance_uri = instance_url.toString();
}

QString AbstractAccount::instanceUri() const
{
    return m_instance_uri;
}

void AbstractAccount::setToken(const QString &authcode)
{
    const QUrl tokenUrl = getTokenUrl();
    QUrlQuery q = buildOAuthQuery();

    q.addQueryItem(QStringLiteral("client_secret"), m_client_secret);
    q.addQueryItem(QStringLiteral("redirect_uri"), QStringLiteral("tokodon://oauth"));
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    q.addQueryItem(QStringLiteral("code"), authcode);

    post(tokenUrl, q, false, this, [=](QNetworkReply *reply) {
        auto data = reply->readAll();
        auto doc = QJsonDocument::fromJson(data);

        setAccessToken(doc.object()["access_token"_L1].toString());
    });
}

void AbstractAccount::mutatePost(const QString &id, const QString &verb, bool deliver_home)
{
    const QUrl mutation_url = apiUrl(QStringLiteral("/api/v1/statuses/%1/%2").arg(id, verb));
    const QJsonDocument doc;

    post(mutation_url, doc, true, this, [=](QNetworkReply *reply) {
        const auto data = reply->readAll();
        const auto doc = QJsonDocument::fromJson(data);

        if (deliver_home) {
            QList<Post *> posts;
            auto obj = doc.object();

            auto p = new Post(this, obj, this);
            posts.push_back(p);

            Q_EMIT fetchedTimeline(QStringLiteral("home"), posts);
        }
    });
}

void AbstractAccount::favorite(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("favourite"));
}

void AbstractAccount::unfavorite(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("unfavourite"));
}

void AbstractAccount::repeat(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("reblog"), true);
}

void AbstractAccount::unrepeat(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("unreblog"));
}

void AbstractAccount::bookmark(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("bookmark"), true);
}

void AbstractAccount::unbookmark(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("unbookmark"));
}

void AbstractAccount::pin(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("pin"), true);
}

void AbstractAccount::unpin(Post *p)
{
    mutatePost(p->postId(), QStringLiteral("unpin"));
}

// It seemed clearer to keep this logic separate from the general instance metadata collection, on the off chance
// that it might need to be extended later on.
static AbstractAccount::AllowedContentType parseVersion(const QString &instanceVer)
{
    using ContentType = AbstractAccount::AllowedContentType;

    unsigned int result = ContentType::PlainText;
    if (instanceVer.contains("glitch"_L1)) {
        result |= ContentType::Markdown | ContentType::Html;
    }

    return static_cast<ContentType>(result);
}

static QMap<QString, AbstractAccount::AllowedContentType> stringToContentType = {
    {QStringLiteral("text/plain"), AbstractAccount::AllowedContentType::PlainText},
    {QStringLiteral("text/bbcode"), AbstractAccount::AllowedContentType::BBCode},
    {QStringLiteral("text/html"), AbstractAccount::AllowedContentType::Html},
    {QStringLiteral("text/markdown"), AbstractAccount::AllowedContentType::Markdown},
};

static AbstractAccount::AllowedContentType parsePleromaInfo(const QJsonDocument &doc)
{
    using ContentType = AbstractAccount::AllowedContentType;
    unsigned int result = ContentType::PlainText;

    auto obj = doc.object();
    if (obj.contains("metadata"_L1)) {
        auto metadata = obj["metadata"_L1].toObject();
        if (!metadata.contains("postFormats"_L1))
            return static_cast<ContentType>(result);

        auto formats = metadata["postFormats"_L1].toArray();

        for (auto c : formats) {
            auto fmt = c.toString();

            if (!stringToContentType.contains(fmt))
                continue;

            result |= (unsigned int)stringToContentType[fmt];
        }
    }

    return static_cast<ContentType>(result);
}

void AbstractAccount::fetchInstanceMetadata()
{
    get(
        apiUrl(QStringLiteral("/api/v2/instance")),
        false,
        this,
        [=](QNetworkReply *reply) {
            if (200 != reply->attribute(QNetworkRequest::HttpStatusCodeAttribute))
                return;

            const auto data = reply->readAll();
            const auto doc = QJsonDocument::fromJson(data);

            if (!doc.isObject())
                return;

            const auto obj = doc.object();

            if (obj.contains("configuration"_L1)) {
                const auto configObj = obj["configuration"_L1].toObject();

                if (configObj.contains("statuses"_L1)) {
                    const auto statusConfigObj = configObj["statuses"_L1].toObject();
                    m_maxPostLength = statusConfigObj["max_characters"_L1].toInt();
                    m_charactersReservedPerUrl = statusConfigObj["characters_reserved_per_url"_L1].toInt();
                }
            }

            // One can only hope that there will always be a version attached
            if (obj.contains("version"_L1)) {
                m_allowedContentTypes = parseVersion(obj["version"_L1].toString());
            }

            // Pleroma/Akkoma may report maximum post characters here, instead
            if (obj.contains("max_toot_chars"_L1)) {
                m_maxPostLength = obj["max_toot_chars"_L1].toInt();
            }

            // Pleroma/Akkoma can report higher poll limits
            if (obj.contains("poll_limits"_L1)) {
                m_maxPollOptions = obj["poll_limits"_L1].toObject()["max_options"_L1].toInt();
            }

            // Other instance of poll options
            if (obj.contains("polls"_L1)) {
                m_maxPollOptions = obj["polls"_L1].toObject()["max_options"_L1].toInt();
            }

            if (obj.contains("registrations"_L1)) {
                m_registrationsOpen = obj["registrations"_L1].toObject()["enabled"_L1].toBool();
                m_registrationMessage = obj["registrations"_L1].toObject()["message"_L1].toString();
            }

            m_supportsLocalVisibility = obj.contains("pleroma"_L1);

            m_instance_name = obj["title"_L1].toString();

            Q_EMIT fetchedInstanceMetadata();
        },
        [=](QNetworkReply *) {
            // Fall back to v1 instance information
            // TODO: a lot of this can be merged with v2 handling
            get(apiUrl(QStringLiteral("/api/v1/instance")), false, this, [=](QNetworkReply *reply) {
                if (200 != reply->attribute(QNetworkRequest::HttpStatusCodeAttribute))
                    return;

                const auto data = reply->readAll();
                const auto doc = QJsonDocument::fromJson(data);

                if (!doc.isObject())
                    return;

                const auto obj = doc.object();

                if (obj.contains("configuration"_L1)) {
                    const auto configObj = obj["configuration"_L1].toObject();

                    if (configObj.contains("statuses"_L1)) {
                        const auto statusConfigObj = configObj["statuses"_L1].toObject();
                        m_maxPostLength = statusConfigObj["max_characters"_L1].toInt();
                        m_charactersReservedPerUrl = statusConfigObj["characters_reserved_per_url"_L1].toInt();
                    }
                }

                // One can only hope that there will always be a version attached
                if (obj.contains("version"_L1)) {
                    m_allowedContentTypes = parseVersion(obj["version"_L1].toString());
                }

                // Pleroma/Akkoma may report maximum post characters here, instead
                if (obj.contains("max_toot_chars"_L1)) {
                    m_maxPostLength = obj["max_toot_chars"_L1].toInt();
                }

                // Pleroma/Akkoma can report higher poll limits
                if (obj.contains("poll_limits"_L1)) {
                    m_maxPollOptions = obj["poll_limits"_L1].toObject()["max_options"_L1].toInt();
                }

                // Other instance of poll options
                if (obj.contains("polls"_L1)) {
                    m_maxPollOptions = obj["polls"_L1].toObject()["max_options"_L1].toInt();
                }

                m_registrationsOpen = obj["registrations"_L1].toBool();

                m_supportsLocalVisibility = obj.contains("pleroma"_L1);

                m_instance_name = obj["title"_L1].toString();

                Q_EMIT fetchedInstanceMetadata();
            });
        });

    get(apiUrl(QStringLiteral("/nodeinfo/2.1.json")), false, this, [=](QNetworkReply *reply) {
        const auto data = reply->readAll();
        const auto doc = QJsonDocument::fromJson(data);

        m_allowedContentTypes = parsePleromaInfo(doc);
        Q_EMIT fetchedInstanceMetadata();
    });

    fetchCustomEmojis();
}

void AbstractAccount::invalidate()
{
    Q_EMIT invalidated();
}

void AbstractAccount::invalidatePost(Post *p)
{
    Q_EMIT invalidatedPost(p);
}

QUrl AbstractAccount::streamingUrl(const QString &stream)
{
    QUrl url = apiUrl(QStringLiteral("/api/v1/streaming"));
    url.setQuery(QUrlQuery{
        {QStringLiteral("access_token"), m_token},
        {QStringLiteral("stream"), stream},
    });
    url.setScheme(QStringLiteral("wss"));

    return url;
}

void AbstractAccount::handleNotification(const QJsonDocument &doc)
{
    const auto obj = doc.object();
    std::shared_ptr<Notification> n = std::make_shared<Notification>(this, obj);

    if (n->type() == Notification::FollowRequest) {
        m_hasFollowRequests = true;
        Q_EMIT hasFollowRequestsChanged();
    }

    Q_EMIT notification(n);
}

void AbstractAccount::executeAction(Identity *identity, AccountAction accountAction, const QJsonObject &extraArguments)
{
    const QHash<AccountAction, QString> accountActionMap = {
        {AccountAction::Follow, QStringLiteral("/follow")},
        {AccountAction::Unfollow, QStringLiteral("/unfollow")},
        {AccountAction::Block, QStringLiteral("/block")},
        {AccountAction::Unblock, QStringLiteral("/unblock")},
        {AccountAction::Mute, QStringLiteral("/mute")},
        {AccountAction::Unmute, QStringLiteral("/unmute")},
        {AccountAction::Feature, QStringLiteral("/pin")},
        {AccountAction::Unfeature, QStringLiteral("/unpin")},
        {AccountAction::Note, QStringLiteral("/note")},
    };

    const auto apiCall = accountActionMap[accountAction];

    const auto accountId = identity->id();
    const QString accountApiUrl = QStringLiteral("/api/v1/accounts/") + accountId + apiCall;
    const QJsonDocument doc(extraArguments);

    post(apiUrl(accountApiUrl), doc, true, this, [=](QNetworkReply *reply) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto jsonObj = doc.object();

        // Check if the request failed due to one account blocking the other
        if (!jsonObj.value("error"_L1).isUndefined()) {
            const QHash<AccountAction, QString> accountActionMap = {
                {AccountAction::Follow, i18n("Could not follow account")},
                {AccountAction::Unfollow, i18n("Could not unfollow account")},
                {AccountAction::Block, i18n("Could not block account")},
                {AccountAction::Unblock, i18n("Could not unblock account")},
                {AccountAction::Mute, i18n("Could not mute account")},
                {AccountAction::Unmute, i18n("Could not unmute account")},
                {AccountAction::Feature, i18n("Could not feature account")},
                {AccountAction::Unfeature, i18n("Could not unfeature account")},
                {AccountAction::Note, i18n("Could not edit note about an account")},
            };
            const auto errorMessage = accountActionMap[accountAction];
            Q_EMIT errorOccured(errorMessage);
            return;
        }
        // If returned json obj is not an error, it's a relationship status.
        // Returned relationship should have a value of true
        // under either the "following" or "requested" keys.
        auto relationship = identity->relationship();
        relationship->updateFromJson(jsonObj);

        Q_EMIT identity->relationshipChanged();
    });
}

void AbstractAccount::followAccount(Identity *identity, bool reblogs, bool notify)
{
    executeAction(identity,
                  AccountAction::Follow,
                  {
                      {QStringLiteral("reblogs"), reblogs},
                      {QStringLiteral("notify"), notify},
                  });
}

void AbstractAccount::unfollowAccount(Identity *identity)
{
    executeAction(identity, AccountAction::Unfollow);
}

void AbstractAccount::blockAccount(Identity *identity)
{
    executeAction(identity, AccountAction::Block);
}

void AbstractAccount::unblockAccount(Identity *identity)
{
    executeAction(identity, AccountAction::Unblock);
}

void AbstractAccount::muteAccount(Identity *identity, bool notifications, int duration)
{
    executeAction(identity, AccountAction::Mute, {{QStringLiteral("notifcations"), notifications}, {QStringLiteral("duration"), duration}});
}

void AbstractAccount::unmuteAccount(Identity *identity)
{
    executeAction(identity, AccountAction::Unmute);
}

void AbstractAccount::featureAccount(Identity *identity)
{
    executeAction(identity, AccountAction::Feature);
}

void AbstractAccount::unfeatureAccount(Identity *identity)
{
    executeAction(identity, AccountAction::Unfeature);
}

void AbstractAccount::addNote(Identity *identity, const QString &note)
{
    if (note.isEmpty()) {
        executeAction(identity, AccountAction::Note);
    } else {
        executeAction(identity, AccountAction::Note, {{QStringLiteral("comment"), note}});
    }
}

void AbstractAccount::mutateRemotePost(const QString &url, const QString &verb)
{
    NetworkController::instance().requestRemoteObject(this, url, [=](QNetworkReply *reply) {
        const auto searchResult = QJsonDocument::fromJson(reply->readAll()).object();
        const auto statuses = searchResult[QStringLiteral("statuses")].toArray();
        const auto accounts = searchResult[QStringLiteral("accounts")].toArray();

        // TODO: emit error when the mutation has failed, no post is available on this account's server
        if (!statuses.isEmpty()) {
            const auto status = statuses[0].toObject();

            if (verb == QStringLiteral("reply")) {
                // TODO: we can't delete this immediately, will need some smarter cleanup in the PostEditorBackend
                Post *post = new Post(this, this);
                post->fromJson(status);
                Q_EMIT Navigation::instance().replyTo(post->postId(), post->mentions(), post->visibility(), post->getAuthorIdentity(), post);
            } else {
                const QString localID = status["id"_L1].toString();
                mutatePost(localID, verb);
            }
        }
    });
}

void AbstractAccount::fetchOEmbed(const QString &id, Identity *identity)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("url"), QStringLiteral("%1/@%2/%3").arg(m_instance_uri, identity->username(), id));

    QUrl oembedUrl = apiUrl(QStringLiteral("/api/oembed"));
    oembedUrl.setQuery(query);

    get(oembedUrl, false, this, [this](QNetworkReply *reply) {
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.object().contains("html"_L1)) {
            Q_EMIT fetchedOEmbed(doc.object()["html"_L1].toString());
        }
    });
}

bool AbstractAccount::isRegistered() const
{
    return !m_client_id.isEmpty() && !m_client_secret.isEmpty();
}

bool AbstractAccount::registrationsOpen() const
{
    return m_registrationsOpen;
}

QString AbstractAccount::registrationMessage() const
{
    return m_registrationMessage;
}

QString AbstractAccount::settingsGroupName() const
{
    return AccountManager::settingsGroupName(m_name, m_instance_uri);
}

QString AbstractAccount::clientSecretKey() const
{
    return AccountManager::clientSecretKey(settingsGroupName());
}

QString AbstractAccount::accessTokenKey() const
{
    return AccountManager::accessTokenKey(settingsGroupName());
}

void AbstractAccount::fetchCustomEmojis()
{
    m_customEmojis.clear();

    get(apiUrl(QStringLiteral("/api/v1/custom_emojis")), false, this, [=](QNetworkReply *reply) {
        if (200 != reply->attribute(QNetworkRequest::HttpStatusCodeAttribute))
            return;

        const auto data = reply->readAll();
        const auto doc = QJsonDocument::fromJson(data);

        if (!doc.isArray())
            return;

        const auto array = doc.array();

        for (auto emojiObj : array) {
            if (!emojiObj.isObject()) {
                continue;
            }

            CustomEmoji customEmoji{};
            customEmoji.shortcode = emojiObj[QStringLiteral("shortcode")].toString();
            customEmoji.url = emojiObj[QStringLiteral("url")].toString();

            m_customEmojis.push_back(customEmoji);
        }

        Q_EMIT fetchedCustomEmojis();
    });
}

QList<CustomEmoji> AbstractAccount::customEmojis() const
{
    return m_customEmojis;
}

#include "moc_abstractaccount.cpp"