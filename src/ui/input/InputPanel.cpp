#include "InputPanel.h"
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QSizePolicy>
#include <QTextEdit>
#include <QVBoxLayout>
#include "AttachmentButton.h"
#include "VoiceButton.h"

namespace CodeHex {

// Custom QTextEdit that sends on Ctrl+Enter
class ExpandingTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit ExpandingTextEdit(QWidget* parent = nullptr) : QTextEdit(parent) {}

    QSize sizeHint() const override {
        return {400, 60};
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
            event->modifiers() & Qt::ControlModifier) {
            emit sendTriggered();
            return;
        }
        QTextEdit::keyPressEvent(event);
    }

signals:
    void sendTriggered();
};

InputPanel::InputPanel(AudioRecorder* recorder, QWidget* parent) : QWidget(parent) {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(8, 4, 8, 4);
    outerLayout->setSpacing(4);

    // Text input
    auto* te = new ExpandingTextEdit(this);
    te->setObjectName("inputTextEdit");
    te->setPlaceholderText("Type a message… (Ctrl+Enter to send)");
    te->setMinimumHeight(48);
    te->setMaximumHeight(120);
    te->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_textEdit = te;
    outerLayout->addWidget(m_textEdit);

    // Buttons row
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(6);

    m_attachBtn = new AttachmentButton(this);
    m_voiceBtn  = new VoiceButton(recorder, this);

    m_stopBtn = new QPushButton("⏹ Stop", this);
    m_stopBtn->setObjectName("stopBtn");
    m_stopBtn->setEnabled(false);
    m_stopBtn->setFixedWidth(80);

    m_sendBtn = new QPushButton("Send ▶", this);
    m_sendBtn->setObjectName("sendBtn");
    m_sendBtn->setEnabled(false);
    m_sendBtn->setFixedWidth(90);
    m_sendBtn->setDefault(true);

    btnRow->addWidget(m_attachBtn);
    btnRow->addWidget(m_voiceBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(m_sendBtn);
    outerLayout->addLayout(btnRow);

    connect(m_textEdit, &QTextEdit::textChanged, this, &InputPanel::onTextChanged);
    connect(m_sendBtn,  &QPushButton::clicked,   this, &InputPanel::onSendClicked);
    connect(m_stopBtn,  &QPushButton::clicked,   this, &InputPanel::stopRequested);
    connect(m_voiceBtn, &VoiceButton::voiceMessageReady,
            this, &InputPanel::onVoiceMessageReady);
    connect(te, &ExpandingTextEdit::sendTriggered, this, &InputPanel::onSendClicked);
}

void InputPanel::setEnabled(bool enabled) {
    m_textEdit->setEnabled(enabled);
    m_attachBtn->setEnabled(enabled);
    m_voiceBtn->setEnabled(enabled);
    onTextChanged();  // refresh send button state
}

void InputPanel::setSendEnabled(bool enabled) {
    m_sendBtn->setEnabled(enabled);
}

void InputPanel::setStopEnabled(bool enabled) {
    m_stopBtn->setEnabled(enabled);
}

void InputPanel::clearInput() {
    m_textEdit->clear();
    m_attachBtn->clearAttachments();
}

void InputPanel::onSendClicked() {
    const QString text = m_textEdit->toPlainText().trimmed();
    const QList<Attachment> att = m_attachBtn->pendingAttachments();
    if (text.isEmpty() && att.isEmpty()) return;
    emit sendRequested(text, att);
    clearInput();
}

void InputPanel::onTextChanged() {
    const bool hasContent = !m_textEdit->toPlainText().trimmed().isEmpty() ||
                            !m_attachBtn->pendingAttachments().isEmpty();
    m_sendBtn->setEnabled(hasContent);
}

void InputPanel::onVoiceMessageReady(const QString& path) {
    Attachment a;
    a.filePath = path;
    a.mimeType = "audio/wav";
    a.type = Attachment::Type::Audio;
    // Send immediately as a voice message
    emit sendRequested({}, {a});
}

}  // namespace CodeHex

// Q_OBJECT in .cpp requires this include for AUTOMOC
#include "InputPanel.moc"
