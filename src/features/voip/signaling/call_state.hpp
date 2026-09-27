#pragma once

#include <QObject>

namespace voip {
namespace signaling {

enum class CallState
{
    Calling,
    Ringing,
    Connected,
    Reconnect,
    Reject,
    HandUp,
    Exchange,

};

} // namespace signaling
} // namespace voip
