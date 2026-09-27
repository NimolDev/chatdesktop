#include "media_device_manager.hpp"

namespace core {
namespace media {

MediaDeviceManager::MediaDeviceManager(QObject *parent)
    : QObject(parent)
    , m_mediaDevices(this)
    , m_audioInputs(this)
    , m_audioOutputs(this)
    , m_cameras(this)
{
    reloadAudioInputs();
    reloadAudioOutputs();
    reloadCameras();

    connect(
        &m_mediaDevices,
        &QMediaDevices::audioInputsChanged,
        this,
        &MediaDeviceManager::reloadAudioInputs
    );

    connect(
        &m_mediaDevices,
        &QMediaDevices::audioOutputsChanged,
        this,
        &MediaDeviceManager::reloadAudioOutputs
    );

    connect(
        &m_mediaDevices,
        &QMediaDevices::videoInputsChanged,
        this,
        &MediaDeviceManager::reloadCameras
    );
}

// MARK: - Models

MediaDeviceModel *MediaDeviceManager::audioInputs()
{
    return &m_audioInputs;
}

MediaDeviceModel *MediaDeviceManager::audioOutputs()
{
    return &m_audioOutputs;
}

MediaDeviceModel *MediaDeviceManager::cameras()
{
    return &m_cameras;
}

// MARK: - IDs

QString MediaDeviceManager::deviceId(
    const QAudioDevice &device
)
{
    return QString::fromLatin1(
        device.id().toBase64()
    );
}

QString MediaDeviceManager::deviceId(
    const QCameraDevice &device
)
{
    return QString::fromLatin1(
        device.id().toBase64()
    );
}

// MARK: - Selected IDs

QString MediaDeviceManager::selectedAudioInputId() const
{
    return m_selectedAudioInputId;
}

QString MediaDeviceManager::selectedAudioOutputId() const
{
    return m_selectedAudioOutputId;
}

QString MediaDeviceManager::selectedCameraId() const
{
    return m_selectedCameraId;
}

// MARK: - Selected indices

int MediaDeviceManager::selectedAudioInputIndex() const
{
    return m_audioInputs.indexOf(
        m_selectedAudioInputId
    );
}

int MediaDeviceManager::selectedAudioOutputIndex() const
{
    return m_audioOutputs.indexOf(
        m_selectedAudioOutputId
    );
}

int MediaDeviceManager::selectedCameraIndex() const
{
    return m_cameras.indexOf(
        m_selectedCameraId
    );
}

// MARK: - Selected native devices

QAudioDevice
MediaDeviceManager::selectedAudioInputDevice() const
{
    const int index = selectedAudioInputIndex();

    if (index >= 0 &&
        index < m_audioInputDevices.size()) {
        return m_audioInputDevices.at(index);
    }

    return QMediaDevices::defaultAudioInput();
}

QAudioDevice
MediaDeviceManager::selectedAudioOutputDevice() const
{
    const int index = selectedAudioOutputIndex();

    if (index >= 0 &&
        index < m_audioOutputDevices.size()) {
        return m_audioOutputDevices.at(index);
    }

    return QMediaDevices::defaultAudioOutput();
}

QCameraDevice
MediaDeviceManager::selectedCameraDevice() const
{
    const int index = selectedCameraIndex();

    if (index >= 0 &&
        index < m_cameraDevices.size()) {
        return m_cameraDevices.at(index);
    }

    return QMediaDevices::defaultVideoInput();
}

// MARK: - Select input

void MediaDeviceManager::selectAudioInput(int index)
{
    if (index < 0 ||
        index >= m_audioInputDevices.size()) {
        return;
    }

    const auto &device =
        m_audioInputDevices.at(index);

    const QString id = deviceId(device);

    if (id == m_selectedAudioInputId) {
        return;
    }

    m_selectedAudioInputId = id;

    emit selectedAudioInputChanged();
    emit audioInputDeviceChanged(device);
}

// MARK: - Select output

void MediaDeviceManager::selectAudioOutput(int index)
{
    if (index < 0 ||
        index >= m_audioOutputDevices.size()) {
        return;
    }

    const auto &device =
        m_audioOutputDevices.at(index);

    const QString id = deviceId(device);

    if (id == m_selectedAudioOutputId) {
        return;
    }

    m_selectedAudioOutputId = id;

    emit selectedAudioOutputChanged();
    emit audioOutputDeviceChanged(device);
}

// MARK: - Select camera

void MediaDeviceManager::selectCamera(int index)
{
    if (index < 0 ||
        index >= m_cameraDevices.size()) {
        return;
    }

    const auto &device =
        m_cameraDevices.at(index);

    const QString id = deviceId(device);

    if (id == m_selectedCameraId) {
        return;
    }

    m_selectedCameraId = id;

    emit selectedCameraChanged();
    emit cameraDeviceChanged(device);
}

// MARK: - Default devices

void MediaDeviceManager::useDefaultAudioInput()
{
    const auto device =
        QMediaDevices::defaultAudioInput();

    if (device.isNull()) {
        return;
    }

    const int index =
        m_audioInputs.indexOf(deviceId(device));

    if (index >= 0) {
        selectAudioInput(index);
    }
}

void MediaDeviceManager::useDefaultAudioOutput()
{
    const auto device =
        QMediaDevices::defaultAudioOutput();

    if (device.isNull()) {
        return;
    }

    const int index =
        m_audioOutputs.indexOf(deviceId(device));

    if (index >= 0) {
        selectAudioOutput(index);
    }
}

void MediaDeviceManager::useDefaultCamera()
{
    const auto device =
        QMediaDevices::defaultVideoInput();

    if (device.isNull()) {
        return;
    }

    const int index =
        m_cameras.indexOf(deviceId(device));

    if (index >= 0) {
        selectCamera(index);
    }
}

// MARK: - Reload inputs

void MediaDeviceManager::reloadAudioInputs()
{
    m_audioInputDevices =
        QMediaDevices::audioInputs();

    QVector<MediaDeviceItem> items;

    items.reserve(
        m_audioInputDevices.size()
    );

    for (const auto &device : std::as_const(m_audioInputDevices)) {

        items.push_back({
            deviceId(device),
            device.description(),
            device.isDefault()
        });
    }

    m_audioInputs.setDevices(
        std::move(items)
    );

    emit selectedAudioInputChanged();

    // First initialization.
    if (m_selectedAudioInputId.isEmpty()) {

        const auto defaultDevice =
            QMediaDevices::defaultAudioInput();

        if (!defaultDevice.isNull()) {
            m_selectedAudioInputId =
                deviceId(defaultDevice);

            emit selectedAudioInputChanged();
            emit audioInputDeviceChanged(defaultDevice);
        }

        return;
    }

    // Previously selected device disappeared.
    if (m_audioInputs.indexOf(
            m_selectedAudioInputId) < 0) {

        const auto defaultDevice =
            QMediaDevices::defaultAudioInput();

        m_selectedAudioInputId =
            defaultDevice.isNull()
                ? QString{}
                : deviceId(defaultDevice);

        emit selectedAudioInputChanged();

        emit audioInputDeviceChanged(defaultDevice);
    }
}

// MARK: - Reload outputs

void MediaDeviceManager::reloadAudioOutputs()
{
    m_audioOutputDevices =
        QMediaDevices::audioOutputs();

    QVector<MediaDeviceItem> items;

    items.reserve(
        m_audioOutputDevices.size()
    );

    for (const auto &device : std::as_const (m_audioOutputDevices)) {

        items.push_back({
            deviceId(device),
            device.description(),
            device.isDefault()
        });
    }

    m_audioOutputs.setDevices(
        std::move(items)
    );

    emit selectedAudioOutputChanged();

    if (m_selectedAudioOutputId.isEmpty()) {

        const auto defaultDevice =
            QMediaDevices::defaultAudioOutput();

        if (!defaultDevice.isNull()) {
            m_selectedAudioOutputId =
                deviceId(defaultDevice);

            emit selectedAudioOutputChanged();
            emit audioOutputDeviceChanged(defaultDevice);
        }

        return;
    }

    if (m_audioOutputs.indexOf(
            m_selectedAudioOutputId) < 0) {

        const auto defaultDevice =
            QMediaDevices::defaultAudioOutput();

        m_selectedAudioOutputId =
            defaultDevice.isNull()
                ? QString{}
                : deviceId(defaultDevice);

        emit selectedAudioOutputChanged();

        emit audioOutputDeviceChanged(defaultDevice);
    }
}

// MARK: - Reload cameras

void MediaDeviceManager::reloadCameras()
{
    m_cameraDevices =
        QMediaDevices::videoInputs();

    QVector<MediaDeviceItem> items;

    items.reserve(
        m_cameraDevices.size()
    );

    for (const auto &device : std::as_const(m_cameraDevices)) {

        items.push_back({
            deviceId(device),
            device.description(),
            device.isDefault()
        });
    }

    m_cameras.setDevices(
        std::move(items)
    );

    emit selectedCameraChanged();

    if (m_selectedCameraId.isEmpty()) {

        const auto defaultDevice =
            QMediaDevices::defaultVideoInput();

        if (!defaultDevice.isNull()) {
            m_selectedCameraId =
                deviceId(defaultDevice);

            emit selectedCameraChanged();
            emit cameraDeviceChanged(defaultDevice);
        }

        return;
    }

    if (m_cameras.indexOf(
            m_selectedCameraId) < 0) {

        const auto defaultDevice =
            QMediaDevices::defaultVideoInput();

        m_selectedCameraId =
            defaultDevice.isNull()
                ? QString{}
                : deviceId(defaultDevice);

        emit selectedCameraChanged();

        emit cameraDeviceChanged(defaultDevice);
    }
}

} // namespace media
} // namespace core