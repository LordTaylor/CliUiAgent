#pragma once
#include <QList>
#include <QWidget>
#include "../../data/Attachment.h"

class QTextEdit;
class QPushButton;

namespace CodeHex {

class AttachmentButton;
class VoiceButton;
class AudioRecorder;

class InputPanel : public QWidget {
    Q_OBJECT
public:
    explicit InputPanel(AudioRecorder* recorder, QWidget* parent = nullptr);

    void setEnabled(bool enabled);
    void setSendEnabled(bool enabled);
    void setStopEnabled(bool enabled);
    void clearInput();

signals:
    void sendRequested(const QString& text, const QList<Attachment>& attachments);
    void stopRequested();

private slots:
    void onSendClicked();
    void onTextChanged();
    void onVoiceMessageReady(const QString& path);

private:
    QTextEdit* m_textEdit;
    AttachmentButton* m_attachBtn;
    VoiceButton* m_voiceBtn;
    QPushButton* m_sendBtn;
    QPushButton* m_stopBtn;
};

}  // namespace CodeHex
