#include "AudioPlayer.h"
#include <QUrl>

namespace CodeHex {

AudioPlayer::AudioPlayer(QObject* parent) : QObject(parent) {
    m_player.setAudioOutput(&m_audioOutput);
    m_audioOutput.setVolume(1.0f);
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
                if (state == QMediaPlayer::StoppedState) emit playbackFinished();
            });
}

void AudioPlayer::play(const QString& filePath) {
    m_player.setSource(QUrl::fromLocalFile(filePath));
    m_player.play();
}

void AudioPlayer::stop() {
    m_player.stop();
}

bool AudioPlayer::isPlaying() const {
    return m_player.playbackState() == QMediaPlayer::PlayingState;
}

}  // namespace CodeHex
