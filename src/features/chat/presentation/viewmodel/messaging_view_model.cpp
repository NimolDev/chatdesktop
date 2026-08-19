#include "messaging_view_model.hpp"

MessagingViewModel *MessagingViewModel::s_instance = nullptr;

MessagingViewModel::MessagingViewModel(QObject *parent)
    : QAbstractListModel(parent)
{
    dummy ();
}

MessagingViewModel *MessagingViewModel::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED (scriptEngine);
    Q_ASSERT (s_instance);
    Q_ASSERT (s_instance->thread() == engine->thread());

    QQmlEngine::setObjectOwnership (s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void MessagingViewModel::setInstance(MessagingViewModel *instance)
{
    Q_ASSERT (instance);
    s_instance = instance;
}

int MessagingViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid ()) {
        qDebug() << "Parent invalide";
        return 0;
    }
    return m_messaging.count ();
}

QVariant MessagingViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid () ||
        index.row() < 0 ||
        index.row () >= m_messaging.size ()) {
        return {};
    }

    const auto &messaging = m_messaging[index.row ()];

    switch (role) {
    case IdRole:
        return messaging.id;
    case NameRole:
        return messaging.name;
    case SenderIdRole:
        return messaging.sender_id;
    case MsgRole:
        return messaging.msg;
    case DateRole:
        return messaging.date;
    case MsgTypeRole:
        return messaging.msg_type;
    case SectionRole: {
        int row = index.row ();
        if (row == 0) return QStringLiteral("Yesterday");
        if (row == 1) return QStringLiteral("Today");
         return QStringLiteral ("Now");
    }
        // return "Today";
    default:
        return {};
    }
}

QHash<int, QByteArray> MessagingViewModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { NameRole, "name" },
        { SenderIdRole, "sender_id" },
        { MsgRole, "msg" },
        { DateRole, "date" },
        { MsgTypeRole, "msg_type" },
        { SectionRole, "section" }
    };
}

int MessagingViewModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}


void MessagingViewModel::dummy()
{
    m_messaging = {
        domain::Messaging {
            .id = "1",
            .name = "userA",
            .sender_id = "1",
            .msg = "hi",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "2",
            .name = "userB",
            .sender_id = "2",
            .msg = "Hello",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "3",
            .name = "userA",
            .sender_id = "1",
            .msg = "How are you?",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "4",
            .name = "userB",
            .sender_id = "2",
            .msg = "I'm good, \nhow about you? ",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "4",
            .name = "userB",
            .sender_id = "2",
            .msg = "Where are you now?",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "5",
            .name = "userA",
            .sender_id = "1",
            .msg = "Nice,",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "4",
            .name = "userB",
            .sender_id = "2",
            .msg = "Where are you now?",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "5",
            .name = "userA",
            .sender_id = "1",
            .msg = "Nice,",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "4",
            .name = "userB",
            .sender_id = "2",
            .msg = "Where are you now?",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "5",
            .name = "userA",
            .sender_id = "1",
            .msg = "Nice,",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "4",
            .name = "userB",
            .sender_id = "2",
            .msg = "Where are you now?",
            .msg_type = "a"

        },
        domain::Messaging {
            .id = "5",
            .name = "userA",
            .sender_id = "1",
            .msg = "Nice,",
            .msg_type = "a"

        }

    };
}
