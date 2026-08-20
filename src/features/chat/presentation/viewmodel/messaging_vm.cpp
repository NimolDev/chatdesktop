#include "messaging_vm.hpp"

#include <algorithm>

MessagingViewModel *MessagingViewModel::s_instance = nullptr;

MessagingViewModel::MessagingViewModel(
    std::shared_ptr<domain::usecase::MessageUsecase> usecase,
    QObject *parent
    )
    : m_usecase(std::move (usecase)),
    QAbstractListModel(parent)
{
    connect(
        &m_watcher,
        &QFutureWatcher<domain::entity::Message>::finished,
        this,
        &MessagingViewModel::onFinished
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
    setIsLoading (true);
    if (!m_message.messages.isEmpty ()) {
        beginResetModel ();
        m_message = {};
        endResetModel ();
        qDebug() << "Model not empty ";
    }
    m_watcher.setFuture(m_usecase->execute (user_id));
}

void MessagingViewModel::resetModel()
{
    setIsLoading (true);
    beginResetModel ();
    m_message = {};
    endResetModel ();
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
    case SenderIdRole:
        return messaging.sender_id;
    case RecipientIdRole:
        return messaging.recipient_id;
    case BodyRole:
        return messaging.body;
    case SentAtRole:
        return messaging.sent_at;
    case SectionRole:
        return sectionForDate (messaging.sent_at);
    default:
        return {};
    }
}

QHash<int, QByteArray> MessagingViewModel::roleNames() const
{
    return {
        { IdRole, "id" },
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

void MessagingViewModel::onFinished()
{
    auto messages = m_watcher.result ();

    std::stable_sort(
        messages.messages.begin(),
        messages.messages.end(),
        [](const domain::entity::Item &left,
           const domain::entity::Item &right) {
            const auto leftDate = QDateTime::fromString(left.sent_at, Qt::ISODate);
            const auto rightDate = QDateTime::fromString(right.sent_at, Qt::ISODate);

            if (!leftDate.isValid())
                return false;
            if (!rightDate.isValid())
                return true;

            return leftDate < rightDate;
        }
    );

    beginResetModel ();
    m_message = std::move (messages);
    endResetModel ();
    setIsLoading (false);
}

QString MessagingViewModel::sectionForDate(const QString &sentAt) const
{
    const QDateTime dateTime =

        QDateTime::fromString(sentAt, Qt::ISODate);

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
