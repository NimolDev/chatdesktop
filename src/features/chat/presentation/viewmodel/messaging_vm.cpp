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
        [this](const domain::entity::MessageItem &payload) {
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
        [this](const domain::entity::MessageItem &payload) {
            // messageMapping (payload);
            if (normalizedId(payload.recipient_id) == m_activeConversationId)
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

    m_usecase->execute(user_id, 1).then(
        this,
        [this, requestId](domain::entity::MessageResponse messages) {
            applyFetchedMessages(std::move(messages), requestId);
        });
}

void MessagingViewModel::fetchNextMessage()
{
    if (m_loading)
        return;

    if (m_activeConversationId.isEmpty()
        || m_message.page < 1
        || m_message.page >= m_message.total_pages) {
        emit olderMessagesLoaded(0);
        return;
    }

    const quint64 requestId = m_fetchRequestId;
    const int nextPage = m_message.page + 1;
    setIsLoading (true);
    m_usecase->execute(m_activeConversationId, nextPage)
        .then(this,
              [this, requestId, nextPage](domain::entity::MessageResponse response) {
                  if (requestId != m_fetchRequestId)
                      return;

                  if (response.page < 1) {
                      setIsLoading(false);
                      emit olderMessagesLoaded(0);
                      return;
                  }

                  QList<QString> displayDates;
                  QList<QString> sections;
                  displayDates.reserve(response.messages.size());
                  sections.reserve(response.messages.size());
                  for (const auto &message : std::as_const(response.messages)) {
                      displayDates.append(
                          core::utils::formateConversationDate(message.body.timestamp));
                      sections.append(sectionForDate(message.body.timestamp));
                  }

                  const int insertedCount = response.messages.size();
                  if (insertedCount > 0) {
                      const int last = insertedCount - 1;
                      beginInsertRows(QModelIndex(), 0, last);
                      m_message.messages = response.messages + m_message.messages;
                      m_displayDates = displayDates + m_displayDates;
                      m_sections = sections + m_sections;
                      endInsertRows();
                  }

                  m_message.page = response.page > 0 ? response.page : nextPage;
                  m_message.limit = response.limit;
                  m_message.total = response.total;
                  m_message.total_pages = response.total_pages;
                  setIsLoading (false);
                  emit olderMessagesLoaded(insertedCount);
                  // emit messageChanged ();
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
    setIsLoading (false);
    emit olderMessagesLoaded(0);
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
    return m_message.messages.size ();
}

QVariant MessagingViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid () ||
        index.row() < 0 ||
        index.row () >= m_message.messages.size ()) {
        return {};
    }

    const auto &messaging = m_message.messages[index.row ()];

    switch (role) {
    case IdRole:
        return messaging.id;
    case IsMineRole:
        return messaging.is_mine;
    case SenderIdRole:
        return messaging.sender_id;
    case RecipientIdRole:
        return messaging.recipient_id;
    case BodyRole:
        return messaging.body.content.text;
    case SentAtRole:
        return m_displayDates.value(index.row());
    case SectionRole:
        return m_sections.value(index.row());
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

void MessagingViewModel::insertMessage(const domain::entity::MessageItem &payload)
{
    int row = m_message.messages.count ();

    beginInsertRows (QModelIndex(), row, row);
    m_message.messages.append (payload);
    m_displayDates.append(core::utils::formateConversationDate(payload.body.timestamp));
    m_sections.append(sectionForDate(payload.body.timestamp));
    endInsertRows ();
    emit messageChanged();
}

void MessagingViewModel::deleteMessage(QList<int> rows)
{
    std::sort (rows.begin (), rows.end (), std::greater<int>());
    rows.erase (std::unique(rows.begin (), rows.end ()), rows.end ());
    for (int row : rows) {
        if (row < 0 || row >= m_message.messages.count ()) {
            continue;
        }
        beginRemoveRows (QModelIndex(), row, row);
        m_message.messages.remove (row);
        m_displayDates.remove(row);
        m_sections.remove(row);
        endRemoveRows ();
    }
}

void MessagingViewModel::applyFetchedMessages(
   domain::entity::MessageResponse messages,
    quint64 requestId)
{
    // A slower request for the previously selected conversation must never
    // replace the messages of the conversation selected after it.
    if (requestId != m_fetchRequestId)
        return;

    QList<QString> displayDates;
    QList<QString> sections;
    displayDates.reserve(messages.messages.size());
    sections.reserve(messages.messages.size());
    for (const auto &message : std::as_const(messages.messages)) {
        displayDates.append(
            core::utils::formateConversationDate(message.body.timestamp));
        sections.append(sectionForDate(message.body.timestamp));
    }

    beginResetModel ();
    m_message = std::move (messages);
    m_displayDates = std::move(displayDates);
    m_sections = std::move(sections);
    endResetModel ();
    setIsLoading (false);
    emit initialMessagesLoaded();
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
