#include "AudioRecorder.h"
#include <QDataStream>
#include <QDebug>
#include <QMediaDevices>

namespace CodeHex {

AudioRecorder::AudioRecorder(QObject* parent) : QObject(parent) {
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);
}

AudioRecorder::~AudioRecorder() {
    stopRecording();
}

void AudioRecorder::startRecording(const QString& outputPath) {
    if (m_source && m_source->state() == QAudio::ActiveState) return;

    m_outputPath = outputPath;
    m_outputFile.setFileName(outputPath);
    if (!m_outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred("Cannot open output file: " + outputPath);
        return;
    }

    m_dataByteCount = 0;
    writeWavHeader();  // placeholder header, filled on stop

    const QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    if (!defaultDevice.isFormatSupported(m_format)) {
        emit errorOccurred("Audio format not supported by default input device.");
        m_outputFile.close();
        return;
    }

    m_source = new QAudioSource(defaultDevice, m_format, this);
    m_audioDevice = m_source->start();
    if (!m_audioDevice) {
        emit errorOccurred("Failed to start audio input.");
        m_outputFile.close();
        return;
    }

    connect(m_audioDevice, &QIODevice::readyRead, this, &AudioRecorder::onAudioDataReady);
    emit recordingStarted();
}

void AudioRecorder::stopRecording() {
    if (!m_source) return;
    m_source->stop();
    m_audioDevice = nullptr;
    delete m_source;
    m_source = nullptr;

    if (m_outputFile.isOpen()) {
        finalizeWav();
        m_outputFile.close();
        emit recordingFinished(m_outputPath);
    }
}

bool AudioRecorder::isRecording() const {
    return m_source && m_source->state() == QAudio::ActiveState;
}

qreal AudioRecorder::currentLevel() const { return m_currentLevel; }

void AudioRecorder::onAudioDataReady() {
    if (!m_audioDevice) return;
    const QByteArray data = m_audioDevice->readAll();
    m_outputFile.write(data);
    m_dataByteCount += data.size();

    // Calculate RMS level for VU meter
    if (!data.isEmpty()) {
        const auto* samples = reinterpret_cast<const qint16*>(data.constData());
        const int count = data.size() / 2;
        double sum = 0;
        for (int i = 0; i < count; ++i) sum += samples[i] * samples[i];
        m_currentLevel = qBound(0.0, qSqrt(sum / count) / 32767.0, 1.0);
        emit levelChanged(m_currentLevel);
    }
}

void AudioRecorder::writeWavHeader() {
    // Write 44-byte placeholder WAV header
    QDataStream out(&m_outputFile);
    out.setByteOrder(QDataStream::LittleEndian);
    // RIFF chunk
    m_outputFile.write("RIFF");
    out << quint32(0);         // total size placeholder
    m_outputFile.write("WAVE");
    // fmt chunk
    m_outputFile.write("fmt ");
    out << quint32(16);        // chunk size
    out << quint16(1);         // PCM
    out << quint16(m_format.channelCount());
    out << quint32(m_format.sampleRate());
    out << quint32(m_format.sampleRate() * m_format.channelCount() * 2); // byte rate
    out << quint16(m_format.channelCount() * 2);  // block align
    out << quint16(16);        // bits per sample
    // data chunk
    m_outputFile.write("data");
    out << quint32(0);         // data size placeholder
}

void AudioRecorder::finalizeWav() {
    QDataStream out(&m_outputFile);
    out.setByteOrder(QDataStream::LittleEndian);
    // Patch RIFF size
    m_outputFile.seek(4);
    out << quint32(36 + m_dataByteCount);
    // Patch data size
    m_outputFile.seek(40);
    out << quint32(m_dataByteCount);
}

}  // namespace CodeHex
