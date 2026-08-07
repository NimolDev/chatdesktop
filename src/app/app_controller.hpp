#ifndef APP_APP_CONTROLLER_HPP
#define APP_APP_CONTROLLER_HPP

#include <QObject>

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);

signals:
};

#endif // APP_APP_CONTROLLER_HPP
