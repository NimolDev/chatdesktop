#ifndef FEATURES_AUTHENTICATION_PRESENTATION_VIEW_VIEWMODEL_LOGIN_VM_HPP
#define FEATURES_AUTHENTICATION_PRESENTATION_VIEW_VIEWMODEL_LOGIN_VM_HPP

#include "domain/usecase/login_use_case.hpp"
#include <QObject>


class LoginVM : public QObject
{
    Q_OBJECT

public:
    explicit LoginVM(
        std::shared_ptr<domain::usecase::LoginUseCase> usecase,
        QObject *parent = nullptr
        );
    ~LoginVM();

    Q_PROPERTY(bool isLoading READ isLoading WRITE setIsLoading NOTIFY isLoadingChanged FINAL)
    Q_PROPERTY(QString userName READ userName NOTIFY loginSucceeded FINAL)
    Q_PROPERTY(QString userJid READ userJid  NOTIFY loginSucceeded FINAL)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setIsLoggedIn NOTIFY isLoggedInChanged FINAL)

    Q_INVOKABLE void login(const QString user_name, const QString password);

    bool isLoading() const;
    bool isLoggedIn() const;
    QString userName() const;
    QString userJid() const;


signals:
    void loginSucceeded();
    void loginFailed();
    void isLoadingChanged();
    void connecting();
    void isLoggedInChanged();
    void connected();

private:
    void setIsLoading(bool loading);
    void setIsLoggedIn(bool loggedIn);

private:
    std::shared_ptr<domain::usecase::LoginUseCase> m_usecase;

    bool m_loading = false;
    bool m_loggedIn = false;

    QString m_userName;
    QString m_userJid;
};

#endif // FEATURES_AUTHENTICATION_PRESENTATION_VIEW_VIEWMODEL_LOGIN_VM_HPP
