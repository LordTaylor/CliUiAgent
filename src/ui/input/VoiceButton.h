#pragma once
#include <QTimer>
#include <QToolButton>

namespace CodeHex {

class AudioRecorder;

class VoiceButton : public QToolButton {
    Q_OBJECT
public:
    explicit VoiceButton(AudioRecorder* recorder, QWidget* parent = nullptr);

signals:
    void voiceMessageReady(const QString& filePath);

private slots:
    void onClicked();
    void onRecordingFinished(const QString& path);
    void onLevelChanged(qreal level);
    void updateIndicator();

private:
    AudioRecorder* m_recorder;
    QTimer m_blinkTimer;
    bool m_blinkState = false;
    QString m_tempPath;
};

}  // namespace CodeHex
