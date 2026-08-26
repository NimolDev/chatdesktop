#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
// #include "domain/messaging.hpp"
// #include "domain/entity/messags.hpp"
#include "domain/usecase/message_usecase.hpp"
#include "domain/usecase/send_message_usecase.hpp"


class MessagingViewModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        IsMineRole,
        SenderIdRole,
        RecipientIdRole,
        BodyRole,
        SentAtRole,
        SectionRole,
    };
    explicit MessagingViewModel(
        std::shared_ptr<domain::usecase::MessageUsecase> usecase,
        std::shared_ptr<domain::usecase::SendMessageUsecase> msg_usecase,
        QObject *parent = nullptr
        );

    static MessagingViewModel *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(MessagingViewModel *instance);


    Q_PROPERTY(bool isLoading READ isLoading WRITE setIsLoading NOTIFY isLoadingChanged FINAL)

    Q_INVOKABLE void fetchMessage(QString user_id);
    Q_INVOKABLE void resetModel();
    Q_INVOKABLE void sendMessage(const QString &receiver_id, const QString &msg);
    Q_INVOKABLE void deleteMessage(QList<int> rows);


public:
    int rowCount(const QModelIndex &parent = QModelIndex ()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const;


signals:
    void isLoadingChanged();
    void messageChanged();

private:
    void applyFetchedMessages(QList<domain::entity::Payload> messages,
                              quint64 requestId);
    QString sectionForDate(const QString &sentAt) const;
    static QString normalizedId(const QString &id);
    void setIsLoading(bool loading);
    void insertMessage(const domain::entity::Payload &payload);
    void messageMapping(const domain::entity::Payload &payload);


private:

    static MessagingViewModel *s_instance;

    std::shared_ptr<domain::usecase::MessageUsecase> m_usecase;
    std::shared_ptr<domain::usecase::SendMessageUsecase> m_msgUsecase;
    QList<domain::entity::Payload> m_message;
    QList<QString> m_displayDates;
    QList<QString> m_sections;

    QString m_activeConversationId;
    quint64 m_fetchRequestId = 0;


private:
    bool m_loading = false;

};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
