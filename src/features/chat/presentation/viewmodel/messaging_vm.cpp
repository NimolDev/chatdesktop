#include "messaging_vm.hpp"

#include <algorithm>

#include "utils/date_time_utils.hpp"
#include "domain/entity/messags.hpp"

MessagingViewModel *MessagingViewModel::s_instance = nullptr;

MessagingViewModel::MessagingViewModel(
    std::shared_ptr<domain::usecase::MessageUsecase> usecase,
    std::shared_ptr<domain::usecase::SendMessageUsecase> msg_usecase,
    QObject *parent
    )
    : m_usecase(std::move (usecase)),
    m_msgUsecase(std::move (msg_usecase)),
    QAbstractListModel(parent)
{
    connect (
        m_usecase.get (),
        &domain::usecase::MessageUsecase::onMessagedReceived,
        this,
        [this](const domain::entity::Payload &payload) {
            qDebug() << "MesaagingVM: Message received";
            // auto const message = messageMapping();
            // messageMapping (payload);
            if (normalizedId(payload.sender_id) == m_activeConversationId)
                insertMessage (payload);
        }
        );
    connect(
        m_usecase.get (),
        &domain::usecase::MessageUsecase::messageSent,
        this,
        [this](const domain::entity::Payload &payload) {
            // messageMapping (payload);
            if (normalizedId(payload.receiver_id) == m_activeConversationId)
                insertMessage (payload);
        }
        );
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

void MessagingViewModel::fetchMessage(QString user_id)
{
    const quint64 requestId = ++m_fetchRequestId;
    m_activeConversationId = normalizedId(user_id);
    setIsLoading (true);

    m_usecase->execute(user_id).then(
        this,
        [this, requestId](QList<domain::entity::Payload> messages) {
            applyFetchedMessages(std::move(messages), requestId);
        });
}

void MessagingViewModel::resetModel()
{
    ++m_fetchRequestId;
    m_activeConversationId.clear();
    setIsLoading (true);
    beginResetModel ();
    m_message = {};
    m_displayDates.clear();
    m_sections.clear();
    endResetModel ();
}

void MessagingViewModel::sendMessage(const QString &receiver_id, const QString &msg)
{
    const QString id = QString(receiver_id).remove(QLatin1Char('-'));
    m_msgUsecase->execute (id, msg);
}

int MessagingViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid ()) {
        qDebug() << "Parent invalide";
        return 0;
    }
    return m_message.size ();
}

QVariant MessagingViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid () ||
        index.row() < 0 ||
        index.row () >= m_message.size ()) {
        return {};
    }

    const auto &messaging = m_message[index.row ()];

    switch (role) {
    case IdRole:
        return messaging.message_id;
    case IsMineRole:
        return messaging.is_mine;
    case SenderIdRole:
        return messaging.sender_id;
    case RecipientIdRole:
        return messaging.receiver_id;
    case BodyRole:
        return messaging.content.text;
    case SentAtRole:
        return m_displayDates.value(index.row());
        // return messaging.timestamp;
    case SectionRole:
        return m_sections.value(index.row());
        // return messaging.timestamp;
        // return core::utils::formateConversationDate (messaging.timestamp);
    default:
        return {};
    }
}

QHash<int, QByteArray> MessagingViewModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { IsMineRole, "isMine" },
        { SenderIdRole, "senderId" },
        { RecipientIdRole, "recipientId" },
        { BodyRole, "body" },
        { SentAtRole, "sentAt" },
        { SectionRole, "section" }
    };
}

bool MessagingViewModel::isLoading() const
{
    return m_loading;
}

void MessagingViewModel::setIsLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit isLoadingChanged ();
}

void MessagingViewModel::insertMessage(const domain::entity::Payload &payload)
{
    int row = m_message.count ();

    beginInsertRows (QModelIndex(), row, row);
    m_message.append (payload);
    m_displayDates.append(core::utils::formateConversationDate(payload.timestamp));
    m_sections.append(sectionForDate(payload.timestamp));
    endInsertRows ();
}

void MessagingViewModel::deleteMessage(QList<int> rows)
{
    std::sort (rows.begin (), rows.end (), std::greater<int>());
    rows.erase (std::unique(rows.begin (), rows.end ()), rows.end ());
    for (int row : rows) {
        if (row < 0 || row >= m_message.count ()) {
            continue;
        }
        beginRemoveRows (QModelIndex(), row, row);
        m_message.remove (row);
        m_displayDates.remove(row);
        m_sections.remove(row);
        endRemoveRows ();
    }
}

void MessagingViewModel::applyFetchedMessages(
    QList<domain::entity::Payload> messages,
    quint64 requestId)
{
    // A slower request for the previously selected conversation must never
    // replace the messages of the conversation selected after it.
    if (requestId != m_fetchRequestId)
        return;

    // beginResetModel ();
    // m_message = {};
    // endResetModel ();

    // resetModel ();


    // std::stable_sort(
    //     messages.begin (),
    //     messages.end(),
    //     [](const domain::entity::Payload &left,
    //        const domain::entity::Payload &right) {
    //         bool leftOk = false;
    //         bool rightOk = false;
    //         const qint64 leftEpoch = left.timestamp.toLongLong(&leftOk);
    //         const qint64 rightEpoch = right.timestamp.toLongLong(&rightOk);

    //         // Keep valid timestamps before invalid values while preserving
    //         // the original order of invalid entries (stable_sort).
    //         if (leftOk != rightOk)
    //             return leftOk;
    //         if (!leftOk)
    //             return false;

    //         return leftEpoch < rightEpoch;
    //     }
    // );

    QList<QString> displayDates;
    QList<QString> sections;
    displayDates.reserve(messages.size());
    sections.reserve(messages.size());
    for (const auto &message : std::as_const(messages)) {
        displayDates.append(
            core::utils::formateConversationDate(message.timestamp));
        sections.append(sectionForDate(message.timestamp));
    }

    beginResetModel ();
    m_message = std::move (messages);
    m_displayDates = std::move(displayDates);
    m_sections = std::move(sections);
    endResetModel ();
    setIsLoading (false);
    emit messageChanged ();
}

QString MessagingViewModel::normalizedId(const QString &id)
{
    QString result = id;
    return result.remove(QLatin1Char('-'));
}

QString MessagingViewModel::sectionForDate(const QString &sentAt) const
{
    bool ok = false;
    const qint64 epochSeconds = sentAt.toLongLong(&ok);
    if (!ok) {
        return {};
    }

    const QDateTime dateTime = QDateTime::fromSecsSinceEpoch(epochSeconds)
                                   .toLocalTime();
    if (!dateTime.isValid()) {
        return {};
    }
    const QDate date = dateTime.date();
    const QDate today = QDate::currentDate();

    if (date == today) {
        return QStringLiteral("Today");
    }

    if (date == today.addDays(-1)) {
        return QStringLiteral("Yesterday");
    }

    return date.toString(QStringLiteral("dd MMM yyyy"));
}

void MessagingViewModel::messageMapping(const domain::entity::Payload &payload)
{
    domain::entity::Message message;

    domain::entity::Item item;
    item.is_mine = payload.is_mine;
    item.id = payload.message_id;
    item.sender_id = payload.sender_id;
    item.recipient_id = payload.receiver_id;
    item.body = payload.content.text;
    item.sent_at = payload.timestamp;
    message.messages.append (item);
    // insertMessage (message);
}
