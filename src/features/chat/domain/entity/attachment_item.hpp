#pragma


#include <QUrl>

namespace domain {


struct AttachmentItem
{
    enum class State {
        Pending,
        Uploading,
        Uploaded,
        Failed,
        Canceled
    };

    QString id;
    QUrl url;
    QString file_name;
    qint64 file_size = 0;
    bool compress;

    double progress = 0.0;
    State state = State::Pending;
    QString error;
};

} // namespace name