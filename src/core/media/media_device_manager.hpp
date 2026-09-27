#pragma once

#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QCameraDevice>

#include "media_device_model.hpp"

namespace core {
namespace media {



class MediaDeviceManager final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
        MediaDeviceModel* audioInputs
            READ audioInputs
                CONSTANT
        )

    Q_PROPERTY(
        MediaDeviceModel* audioOutputs
            READ audioOutputs
                CONSTANT
        )

    Q_PROPERTY(
        MediaDeviceModel* cameras
            READ cameras
                CONSTANT
        )

    Q_PROPERTY(
        QString selectedAudioInputId
            READ selectedAudioInputId
                NOTIFY selectedAudioInputChanged
        )

    Q_PROPERTY(
        QString selectedAudioOutputId
            READ selectedAudioOutputId
                NOTIFY selectedAudioOutputChanged
        )

    Q_PROPERTY(
        QString selectedCameraId
            READ selectedCameraId
                NOTIFY selectedCameraChanged
        )

    Q_PROPERTY(
        int selectedAudioInputIndex
            READ selectedAudioInputIndex
                NOTIFY selectedAudioInputChanged
        )

    Q_PROPERTY(
        int selectedAudioOutputIndex
            READ selectedAudioOutputIndex
                NOTIFY selectedAudioOutputChanged
        )

    Q_PROPERTY(
        int selectedCameraIndex
            READ selectedCameraIndex
                NOTIFY selectedCameraChanged
        )

public:
    explicit MediaDeviceManager(QObject *parent = nullptr);

    [[nodiscard]]
    MediaDeviceModel *audioInputs();

    [[nodiscard]]
    MediaDeviceModel *audioOutputs();

    [[nodiscard]]
    MediaDeviceModel *cameras();

    [[nodiscard]]
    QString selectedAudioInputId() const;

    [[nodiscard]]
    QString selectedAudioOutputId() const;

    [[nodiscard]]
    QString selectedCameraId() const;

    [[nodiscard]]
    int selectedAudioInputIndex() const;

    [[nodiscard]]
    int selectedAudioOutputIndex() const;

    [[nodiscard]]
    int selectedCameraIndex() const;

    [[nodiscard]]
    QAudioDevice selectedAudioInputDevice() const;

    [[nodiscard]]
    QAudioDevice selectedAudioOutputDevice() const;

    [[nodiscard]]
    QCameraDevice selectedCameraDevice() const;

    Q_INVOKABLE void selectAudioInput(int index);
    Q_INVOKABLE void selectAudioOutput(int index);
    Q_INVOKABLE void selectCamera(int index);

    Q_INVOKABLE void useDefaultAudioInput();
    Q_INVOKABLE void useDefaultAudioOutput();
    Q_INVOKABLE void useDefaultCamera();

signals:
    void selectedAudioInputChanged();
    void selectedAudioOutputChanged();
    void selectedCameraChanged();

    void audioInputDeviceChanged(
        const QAudioDevice &device
        );

    void audioOutputDeviceChanged(
        const QAudioDevice &device
        );

    void cameraDeviceChanged(
        const QCameraDevice &device
        );

private:
    void reloadAudioInputs();
    void reloadAudioOutputs();
    void reloadCameras();

    static QString deviceId(const QAudioDevice &device);
    static QString deviceId(const QCameraDevice &device);

    QMediaDevices m_mediaDevices;

    MediaDeviceModel m_audioInputs;
    MediaDeviceModel m_audioOutputs;
    MediaDeviceModel m_cameras;

    QList<QAudioDevice> m_audioInputDevices;
    QList<QAudioDevice> m_audioOutputDevices;
    QList<QCameraDevice> m_cameraDevices;

    QString m_selectedAudioInputId;
    QString m_selectedAudioOutputId;
    QString m_selectedCameraId;
};

} // namespace media
} // namespace core
