#pragma once
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QObject>
#include <QString>

namespace CodeHex {

class AudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayer(QObject* parent = nullptr);

    void play(const QString& filePath);
    void stop();
    bool isPlaying() const;

signals:
    void playbackFinished();

private:
    QMediaPlayer m_player;
    QAudioOutput m_audioOutput;
};

}  // namespace CodeHex
