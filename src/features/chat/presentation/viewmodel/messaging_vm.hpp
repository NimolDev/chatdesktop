#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QFutureWatcher>
// #include "domain/messaging.hpp"
#include "domain/entity/messags.hpp"
#include "domain/usecase/message_usecase.hpp"



class MessagingViewModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        SenderIdRole,
        RecipientIdRole,
        BodyRole,
        SentAtRole,
        SectionRole,
    };
    explicit MessagingViewModel(
        std::shared_ptr<domain::usecase::MessageUsecase> usecase,
        QObject *parent = nullptr
        );

    static MessagingViewModel *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(MessagingViewModel *instance);


    Q_PROPERTY(bool isLoading READ isLoading WRITE setIsLoading NOTIFY isLoadingChanged FINAL)

    Q_INVOKABLE void fetchMessage(QString user_id);
    Q_INVOKABLE void resetModel();



public:
    int rowCount(const QModelIndex &parent = QModelIndex ()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const;



signals:
    void isLoadingChanged();

private:
    void onFinished();
    QString sectionForDate(const QString &sentAt) const;
    void setIsLoading(bool loading);
private:

    static MessagingViewModel *s_instance;

    std::shared_ptr<domain::usecase::MessageUsecase> m_usecase;
    domain::entity::Message m_message;

    QFutureWatcher<domain::entity::Message> m_watcher;
    bool m_loading;
};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
