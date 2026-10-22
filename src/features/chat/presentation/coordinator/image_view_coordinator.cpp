#include "image_view_coordinator.hpp"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlInfo>

namespace presentation {
namespace coordinator {

ImageViewCoordinator::ImageViewCoordinator(QObject *parent)
    : QObject(parent)
{
}

ImageViewCoordinator::~ImageViewCoordinator()
{
    delete m_window.data();
}

void ImageViewCoordinator::open(const QUrl &source, MessagingViewModel *viewModel)
{
    if (!viewModel || source.isEmpty() || !source.isValid()) {
        return;
    }
    if (!m_window) {
        auto *engine = qmlEngine(this);
        if (!engine) {
            qmlWarning(this) << "Image viewer requires a QML engine.";
            return;
        }
        QQmlComponent component(engine);
        component.loadFromModule("Features.Chat", "ImageViewWindow");
        QObject *object = component.createWithInitialProperties({
            {QStringLiteral("visible"), false},
            {QStringLiteral("imageSource"), source},
            {QStringLiteral("messagingVM"), QVariant::fromValue(viewModel)}
        });
        auto *window = qobject_cast<QWindow *>(object);
        if (!window) {
            qmlWarning(this) << "Could not create ImageViewWindow:" << component.errorString();
            delete object;
            return;
        }
        QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
        window->QObject::setParent(this);
        m_window = window;
        m_platform.setup(window);
        m_platform.setWindowFillContent();
    }
    m_window->setProperty("messagingVM", QVariant::fromValue(viewModel));
    QMetaObject::invokeMethod(m_window.data(), "reset");
    m_window->setProperty("imageSource", source);
    QMetaObject::invokeMethod(m_window.data(), "getImage");
    if (m_window->windowState() == Qt::WindowMinimized) {
        m_window->showNormal();
    } else {
        m_window->show();
    }
    m_window->raise();
    m_window->requestActivate();
}

} // namespace coordinator
} // namespace presentation
