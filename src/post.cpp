// SPDX-FileCopyrightText: 2021 kaniini <https://git.pleroma.social/kaniini>
// SPDX-License-Identifier: GPL-3.0-only

#include <QDebug>
#include <QMap>

#include "post.h"
#include "account.h"

static QMap<QString, Attachment::AttachmentType> str_to_att_type = {
    {"image", Attachment::AttachmentType::Image},
    {"gifv", Attachment::AttachmentType::GifV},
    {"video", Attachment::AttachmentType::Video}
};

Attachment::Attachment(Post *parent, QJsonObject &obj)
    : m_parent(parent),
      m_reply(nullptr)
{
    m_type = Unknown;
    if (! obj.contains("type"))
        return;

    m_id = obj["id"].toString();
    m_url = obj["url"].toString();
    m_preview_url = obj["preview_url"].toString();
    m_description = obj["description"].toString();

    // determine type if we can
    auto type = obj["type"].toString();
    if (str_to_att_type.contains (type))
        m_type = str_to_att_type[type];
}

Attachment::~Attachment()
{
    cancelFetchRequest();
}

void Attachment::cancelFetchRequest()
{
    if (! m_reply)
        return;

    m_reply->disconnect();
    m_reply->deleteLater();
}

void Attachment::fetchPreviewImage()
{
    cancelFetchRequest();

    QNetworkRequest request = QNetworkRequest(m_preview_url);
    m_reply = m_parent->m_parent->qnam()->get(request);

    QObject::connect(m_reply, &QNetworkReply::finished, [=] () {
        if (! m_reply->isFinished())
            return;

        auto data = m_reply->readAll();
        m_preview = QImage::fromData(data);

        m_parent->setDirtyAttachment();
    });
}

void Attachment::setDescription(QString desc)
{
    m_description = desc;
    m_parent->updateAttachment(this);
}

static QMap<Post::Visibility, QString> vis_to_str = {
    {Post::Visibility::Public, "public"},
    {Post::Visibility::Unlisted, "unlisted"},
    {Post::Visibility::Private, "private"},
    {Post::Visibility::Direct, "direct"}
};

static QMap<QString, Post::Visibility> str_to_vis = {
    {"public", Post::Visibility::Public},
    {"unlisted", Post::Visibility::Unlisted},
    {"private", Post::Visibility::Private},
    {"direct", Post::Visibility::Direct}
};

Post::Post(Account *parent)
    : m_parent(parent),
      m_is_expanded(true),
      m_attachments_visible(true)
{
    QString vis_str = parent->identity().m_visibility;
    m_visibility = str_to_vis[vis_str];
}

Post::Post(Account *parent, QJsonObject obj)
    : m_parent(parent),
      m_visibility(Post::Visibility::Public)
{
    auto account_doc = obj["account"].toObject();
    auto acct = account_doc["acct"].toString();
    auto reblog_obj = obj["reblog"].toObject();

    if (! obj.contains("reblog") || reblog_obj.isEmpty())
    {
        m_repeat = false;
        m_author_identity = m_parent->identityLookup(acct, account_doc);
    }
    else
    {
        m_repeat = true;

        auto repeat_doc = obj["reblog"].toObject();
        auto repeat_account_doc = repeat_doc["account"].toObject();
        auto repeat_acct = repeat_account_doc["acct"].toString();

        m_author_identity = m_parent->identityLookup(repeat_acct, repeat_account_doc);
        m_repeat_identity = m_parent->identityLookup(acct, account_doc);
    }

    m_subject = obj["spoiler_text"].toString();
    m_content = obj["content"].toString();
    m_post_id =  m_reply_target_id = obj["id"].toString();
    m_is_favorited = obj["favourited"].toBool();
    m_is_repeated = obj["reblogged"].toBool();
    m_is_sensitive = obj["sensitive"].toBool();
    m_link = QUrl(obj["url"].toString());
    m_visibility = str_to_vis[obj["visibility"].toString()];

    QJsonArray mentions;

    if (m_repeat)
    {
        m_visibility = str_to_vis[reblog_obj["visibility"].toString()];
        m_reply_target_id = reblog_obj["id"].toString();
        addAttachments(reblog_obj["media_attachments"].toArray());

        mentions = reblog_obj["mentions"].toArray();
    }
    else
    {
        addAttachments(obj["media_attachments"].toArray());

        mentions = obj["mentions"].toArray();
    }

    for (auto m : mentions)
    {
        QJsonObject o = m.toObject();
        m_mentions.push_back(o["acct"].toString());
    }

    m_attachments_visible = ! m_is_sensitive;
    m_is_expanded = m_subject.isEmpty ();
}

Post::~Post()
{
    m_attachments.clear();
}

void Post::addAttachments(const QJsonArray& attachments)
{
    for (auto attachment_val : attachments)
    {
        auto attachment_obj = attachment_val.toObject();
        auto attachment = std::make_shared<Attachment>(this, attachment_obj);

        m_attachments.push_back(attachment);
    }
}

void Post::fetchAttachmentPreviews()
{
    for (auto att : m_attachments)
        att->fetchPreviewImage();
}

void Post::setDirtyAttachment()
{
    m_parent->invalidatePost(this);
}

QJsonDocument Post::toJsonDocument() const
{
    QJsonObject obj;

    obj["spoiler_text"] = m_subject;
    obj["status"] = m_content;
    obj["content_type"] = m_content_type;
    obj["sensitive"] = m_is_sensitive;
    obj["visibility"] = vis_to_str[m_visibility];

    if (! m_reply_target_id.isEmpty ())
        obj["in_reply_to_id"] = m_reply_target_id;

    auto media_ids = QJsonArray ();
    for (auto att : m_attachments)
    {
         media_ids.append (att->m_id);
    }
    obj["media_ids"] = media_ids;

    return QJsonDocument (obj);
}

void Post::updateAttachment(Attachment *a)
{
    m_parent->updateAttachment(a);
}

static QMap<QString, Notification::Type> str_to_not_type = {
    {"favourite", Notification::Type::Favorite},
    {"follow", Notification::Type::Follow},
    {"mention", Notification::Type::Mention},
    {"reblog", Notification::Type::Repeat}
};

Notification::Notification(Account *parent, QJsonObject &obj)
    : m_account(parent)
{
    QJsonObject account = obj["account"].toObject ();
    QJsonObject status = obj["status"].toObject ();
    auto acct = account["acct"].toString ();
    auto type = obj["type"].toString ();

    m_post = std::make_shared<Post> (m_account, status);
    m_identity = m_account->identityLookup (acct, account);
    m_type = str_to_not_type[type];
}
