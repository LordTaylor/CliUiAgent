#include "VoiceButton.h"
#include <QDir>
#include <QStandardPaths>
#include "../../audio/AudioRecorder.h"

namespace CodeHex {

VoiceButton::VoiceButton(AudioRecorder* recorder, QWidget* parent)
    : QToolButton(parent), m_recorder(recorder) {
    setText("🎤");
    setToolTip("Hold to record / click to stop");
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setObjectName("voiceBtn");

    connect(this, &QToolButton::clicked, this, &VoiceButton::onClicked);
    connect(m_recorder, &AudioRecorder::recordingFinished,
            this, &VoiceButton::onRecordingFinished);
    connect(m_recorder, &AudioRecorder::levelChanged,
            this, &VoiceButton::onLevelChanged);

    m_blinkTimer.setInterval(500);
    connect(&m_blinkTimer, &QTimer::timeout, this, &VoiceButton::updateIndicator);
}

void VoiceButton::onClicked() {
    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
        m_blinkTimer.stop();
        setChecked(false);
        setText("🎤");
    } else {
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        m_tempPath = cacheDir + "/codehex_voice_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
        m_recorder->startRecording(m_tempPath);
        m_blinkTimer.start();
        setChecked(true);
    }
}

void VoiceButton::onRecordingFinished(const QString& path) {
    m_blinkTimer.stop();
    setChecked(false);
    setText("🎤");
    emit voiceMessageReady(path);
}

void VoiceButton::onLevelChanged(qreal /*level*/) {
    // Level-based animation could update button color here
}

void VoiceButton::updateIndicator() {
    m_blinkState = !m_blinkState;
    setText(m_blinkState ? "🔴" : "🎤");
}

}  // namespace CodeHex
