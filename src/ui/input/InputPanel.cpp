#include "InputPanel.h"
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QTextEdit>
#include <QVBoxLayout>
#include "AttachmentButton.h"
#include "VoiceButton.h"

namespace CodeHex {

// Custom QTextEdit that sends on Ctrl+Enter or plain Enter
class ExpandingTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit ExpandingTextEdit(QWidget* parent = nullptr) : QTextEdit(parent) {}

    QSize sizeHint() const override { return {400, 60}; }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        const bool enter = (event->key() == Qt::Key_Return ||
                            event->key() == Qt::Key_Enter);
        
        if (enter && !(event->modifiers() & Qt::ShiftModifier)) {
            emit sendTriggered();
            return;
        }

        if (event->key() == Qt::Key_Up) {
            // Trigger history-up only if we are at the top of the text
            if (textCursor().blockNumber() == 0) {
                emit historyUp();
                return;
            }
        }

        if (event->key() == Qt::Key_Down) {
            // Trigger history-down only if we are at the bottom of the text
            if (textCursor().blockNumber() == document()->blockCount() - 1) {
                emit historyDown();
                return;
            }
        }

        QTextEdit::keyPressEvent(event);
    }

signals:
    void sendTriggered();
    void historyUp();
    void historyDown();
};

InputPanel::InputPanel(AudioRecorder* recorder, QWidget* parent) : QWidget(parent) {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(8, 4, 8, 4);
    outerLayout->setSpacing(4);

    // Attachment badge row — shows filenames of queued attachments
    m_attachBadge = new QLabel(this);
    m_attachBadge->setObjectName("attachBadge");
    m_attachBadge->setWordWrap(true);
    m_attachBadge->setVisible(false);
    outerLayout->addWidget(m_attachBadge);

    // Text input
    auto* te = new ExpandingTextEdit(this);
    te->setObjectName("inputTextEdit");
    te->setPlaceholderText("Type a message… (Enter to send, Shift+Enter for newline)");
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
    connect(te, &ExpandingTextEdit::historyUp, this, [this]() {
        if (m_history.isEmpty()) return;
        
        if (m_historyIndex == -1) {
            m_tempInput = m_textEdit->toPlainText();
            m_historyIndex = m_history.size() - 1;
        } else if (m_historyIndex > 0) {
            m_historyIndex--;
        }

        m_textEdit->setPlainText(m_history.at(m_historyIndex));
        m_textEdit->moveCursor(QTextCursor::End);
    });

    connect(te, &ExpandingTextEdit::historyDown, this, [this]() {
        if (m_historyIndex == -1) return;

        if (m_historyIndex < m_history.size() - 1) {
            m_historyIndex++;
            m_textEdit->setPlainText(m_history.at(m_historyIndex));
        } else {
            m_historyIndex = -1;
            m_textEdit->setPlainText(m_tempInput);
        }
        m_textEdit->moveCursor(QTextCursor::End);
    });
    // Refresh Send button + badge whenever the attachment list changes
    connect(m_attachBtn, &AttachmentButton::attachmentsChanged,
            this, &InputPanel::onAttachmentsChanged);
}

void InputPanel::setEnabled(bool enabled) {
    m_textEdit->setEnabled(enabled);
    m_attachBtn->setEnabled(enabled);
    m_voiceBtn->setEnabled(enabled);
    onTextChanged();
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
    // Badge hidden via onAttachmentsChanged which clearAttachments() triggers
}

void InputPanel::addAttachments(const QList<Attachment>& attachments) {
    m_attachBtn->addAttachments(attachments);
}

void InputPanel::onSendClicked() {
    const QString text = m_textEdit->toPlainText().trimmed();
    const QList<Attachment> att = m_attachBtn->pendingAttachments();
    
    if (text.isEmpty() && att.isEmpty()) return;

    // Handle Commands
    if (text.startsWith("/") && !text.startsWith("//")) {
        QStringList parts = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (!parts.isEmpty()) {
            QString cmd = parts.takeFirst();
            emit commandRequested(cmd, parts);
            clearInput();
            return;
        }
    }

    // Normal Message: Add to history
    if (!text.isEmpty()) {
        if (m_history.isEmpty() || m_history.last() != text) {
            m_history.append(text);
            if (m_history.size() > 50) m_history.removeFirst();
        }
    }
    m_historyIndex = -1;

    emit sendRequested(text, att);
    clearInput();
}

void InputPanel::onTextChanged() {
    const bool hasContent = !m_textEdit->toPlainText().trimmed().isEmpty() ||
                            !m_attachBtn->pendingAttachments().isEmpty();
    m_sendBtn->setEnabled(hasContent);
}

void InputPanel::onAttachmentsChanged(const QList<Attachment>& attachments) {
    onTextChanged();   // refresh Send button state

    if (attachments.isEmpty()) {
        m_attachBadge->setVisible(false);
        m_attachBtn->setToolTip("Attach files or images");
        return;
    }

    // Build a compact list of filenames for the badge
    QStringList names;
    for (const Attachment& a : attachments)
        names << QFileInfo(a.filePath).fileName();

    m_attachBadge->setText("📎 " + names.join("  ·  "));
    m_attachBadge->setVisible(true);
    m_attachBtn->setToolTip(
        QString("%1 file(s) attached:\n  ").arg(attachments.size()) + names.join("\n  "));
}

void InputPanel::onVoiceMessageReady(const QString& path) {
    Attachment a;
    a.filePath = path;
    a.mimeType = "audio/wav";
    a.type = Attachment::Type::Audio;
    emit sendRequested({}, {a});
}

}  // namespace CodeHex

// Q_OBJECT in .cpp requires this include for AUTOMOC
#include "InputPanel.moc"
