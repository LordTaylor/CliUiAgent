#pragma once
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>
#include "../../data/Attachment.h"

class QLabel;
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
    void addAttachments(const QList<Attachment>& attachments);

signals:
    void sendRequested(const QString& text, const QList<Attachment>& attachments);
    void commandRequested(const QString& cmd, const QStringList& args);
    void stopRequested();

private slots:
    void onSendClicked();
    void onTextChanged();
    void onVoiceMessageReady(const QString& path);
    void onAttachmentsChanged(const QList<Attachment>& attachments);

private:
    QLabel*          m_attachBadge;  // shows queued filenames above text input
    QTextEdit*       m_textEdit;
    AttachmentButton* m_attachBtn;
    VoiceButton*     m_voiceBtn;
    QPushButton*     m_sendBtn;
    QPushButton*     m_stopBtn;
    
    // History
    QStringList      m_history;
    int              m_historyIndex = -1;
    QString          m_tempInput;
};

}  // namespace CodeHex
