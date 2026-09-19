#ifndef NOTIFICATION_HPP
#define NOTIFICATION_HPP

#include <QString>

namespace platform {

class Notification
{
public:
    virtual ~Notification() = default;

    virtual void requestPermission() =0;
    virtual void show(
        const QString &title,
        const QString &message
        ) = 0;
};

} // namespace platform


#endif // NOTIFICATION_HPP
