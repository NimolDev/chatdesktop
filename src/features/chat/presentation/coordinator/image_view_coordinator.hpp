#pragma once

#include "platform/platform_main_window.hpp"
#include "presentation/viewmodel/messaging_vm.hpp"

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

namespace presentation {
namespace coordinator {

class ImageViewCoordinator final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ImageViewCoordinator(QObject *parent = nullptr);
    ~ImageViewCoordinator() override;

    Q_INVOKABLE void open(const QUrl &source, MessagingViewModel *viewModel);

private:
    core::platform::PlatformMainWindow m_platform;
    QPointer<QWindow> m_window;
};

} // namespace coordinator
} // namespace presentation
