#pragma once
#include <QAudioFormat>
#include <QAudioSource>
#include <QFile>
#include <QObject>
#include <QString>

namespace CodeHex {

class AudioRecorder : public QObject {
    Q_OBJECT
public:
    explicit AudioRecorder(QObject* parent = nullptr);
    ~AudioRecorder() override;

    void startRecording(const QString& outputPath);
    void stopRecording();
    bool isRecording() const;
    qreal currentLevel() const;  // 0.0 – 1.0 for VU meter

signals:
    void recordingStarted();
    void recordingFinished(const QString& filePath);
    void levelChanged(qreal level);
    void errorOccurred(const QString& error);

private slots:
    void onAudioDataReady();

private:
    void writeWavHeader();
    void finalizeWav();

    QAudioSource* m_source = nullptr;
    QIODevice* m_audioDevice = nullptr;
    QFile m_outputFile;
    QString m_outputPath;
    QAudioFormat m_format;
    qint64 m_dataByteCount = 0;
    qreal m_currentLevel = 0.0;
};

}  // namespace CodeHex
