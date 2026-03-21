/**
 * @file MainWindow_Generation.cpp
 * @brief Streaming/generation UI handlers:
 *        onTokenReceived, onTokenBufferTimeout, onCursorBlink, onResponseComplete,
 *        onGenerationStarted, onGenerationStopped, onStopRequested,
 *        onContextStatsUpdated, onTokenStatsUpdated, onToolApprovalRequested.
 */
#include "MainWindow.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QUuid>
#include "../core/AgentEngine.h"
#include "../core/ChatController.h"
#include "chat/ChatControlBanner.h"
#include "chat/ChatView.h"
#include "chat/MessageModel.h"
#include "chat/ToolApprovalDialog.h"
#include "input/InputPanel.h"

using namespace CodeHex;

void MainWindow::onTokenReceived(const QString& token) {
    m_tokenBuffer += token;
    if (!m_tokenTimer->isActive()) m_tokenTimer->start();
}

void MainWindow::onTokenBufferTimeout() {
    if (m_tokenBuffer.isEmpty()) return;

    const QString tokens = m_tokenBuffer;
    m_tokenBuffer.clear();

    if (!m_hasStreamingMsg) {
        m_streamingText = tokens;
        Message liveMsg;
        liveMsg.id = QUuid::createUuid();
        liveMsg.role = Message::Role::Assistant;
        if (m_controller && m_controller->agent() && m_controller->agent()->isCoVeActive()) {
            liveMsg.isInternal = true;
        }
        liveMsg.contentBlocks.append(CodeBlock{tokens, BlockType::Text});
        liveMsg.contentTypes.append(Message::ContentType::Text);
        liveMsg.timestamp = QDateTime::currentDateTimeUtc();
        m_messageModel->appendMessage(liveMsg);
        m_hasStreamingMsg = true;
    } else {
        m_streamingText += tokens;
        m_messageModel->updateLastMessage(m_streamingText);
    }
    if (m_chatView->autoScrollEnabled()) m_chatView->scrollToBottom();
}

void MainWindow::onCursorBlink() {
    if (!m_hasStreamingMsg) return;
    m_cursorVisible = !m_cursorVisible;
    m_messageModel->updateLastMessage(m_streamingText + (m_cursorVisible ? "▋" : ""));
}

void MainWindow::onResponseComplete(const Message& msg) {
    m_cursorTimer->stop();
    m_tokenTimer->stop();
    m_isStreaming = false;
    m_cursorVisible = false;

    if (!m_tokenBuffer.isEmpty()) onTokenBufferTimeout();

    if (m_hasStreamingMsg && !msg.textFromContentBlocks().isEmpty()) {
        m_messageModel->updateLastMessage(msg.textFromContentBlocks());
    } else if (!m_hasStreamingMsg) {
        Message finalMsg = msg;
        if (msg.contentBlocks.isEmpty() && !msg.rawContent.isEmpty()) {
            CodeBlock textBlock;
            textBlock.type = BlockType::Text;
            textBlock.content = msg.rawContent;
            finalMsg.contentBlocks.append(textBlock);
        }
        if (!finalMsg.contentBlocks.isEmpty()) m_messageModel->appendMessage(finalMsg);
    }

    m_hasStreamingMsg = false;
    m_streamingText.clear();
    m_chatView->scrollToBottom();
    updateTokenLabel();
}

void MainWindow::onGenerationStarted() {
    m_isStreaming = true;
    m_cursorVisible = true;
    m_cursorTimer->start();
    m_chatBanner->setThinking(true);
    m_stopBtn->setEnabled(true);
    m_scrollToBottomBtn->setEnabled(false);
    m_inputPanel->setSendEnabled(false);
    m_inputPanel->setStopEnabled(true);
    m_stopBtn->setVisible(true);
    m_statusLabel->setVisible(true);
    m_statusLabel->setText("Agent is Thinking...");
    m_statusLabel->setStyleSheet(
        m_statusLabel->styleSheet()
            .replace(QRegularExpression("color:[^;]+"), "color: #3B82F6")
            .replace(QRegularExpression("border:[^;]+"), "border: 1px solid #3B82F6"));
    statusBar()->showMessage("Processing…");
}

void MainWindow::onGenerationStopped() {
    m_chatBanner->setThinking(false);
    m_cursorTimer->stop();
    m_isStreaming = false;
    m_cursorVisible = false;
    m_inputPanel->setSendEnabled(true);
    m_inputPanel->setStopEnabled(false);
    m_stopBtn->setVisible(false);
    m_statusLabel->setText("Ready");
    m_statusLabel->setStyleSheet(
        m_statusLabel->styleSheet()
            .replace(QRegularExpression("color:[^;]+"), "color: #9CA3AF")
            .replace(QRegularExpression("border:[^;]+"), "border: 1px solid #4B5563"));
    statusBar()->clearMessage();
    updateTokenLabel();
}

void MainWindow::onStopRequested() {
    m_controller->stopGeneration();
    m_stopBtn->setVisible(false);
}

void MainWindow::onContextStatsUpdated(const CodeHex::ContextManager::ContextStats& stats) {
    if (!m_contextBar || !m_contextLabel) return;

    int percentage = qRound(stats.usagePercentage * 100.0f);
    m_contextBar->setValue(percentage);
    m_contextLabel->setText(QString("CTX: %1%").arg(percentage));

    QString chunkStyle = percentage > 90 ? "background: #EF4444"
                       : percentage > 75 ? "background: #F59E0B"
                                         : "background: #3B82F6";
    m_contextBar->setStyleSheet(
        "QProgressBar { border: 1px solid #3F3F46; border-radius: 4px; background: #18181B; } "
        "QProgressBar::chunk { " + chunkStyle + "; border-radius: 3px; }");

    m_contextBar->setToolTip(QString("Context Usage: %1 / %2 tokens (%3 messages)")
                             .arg(stats.totalTokens).arg(stats.maxTokens).arg(stats.messageCount));
}

void MainWindow::onTokenStatsUpdated(int input, int output) {
    updateTokenLabel(input, output);
}

void MainWindow::onToolApprovalRequested(const CodeHex::ToolCall& call) {
    m_statusLabel->setText("Waiting for Tool Approval: " + call.name);
    m_statusLabel->setStyleSheet(
        m_statusLabel->styleSheet()
            .replace(QRegularExpression("color:[^;]+"), "color: #F59E0B")
            .replace(QRegularExpression("border:[^;]+"), "border: 1px solid #F59E0B"));

    ToolApprovalDialog dlg(call, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_statusLabel->setText("Executing Tool: " + call.name);
        m_statusLabel->setStyleSheet(
            m_statusLabel->styleSheet()
                .replace(QRegularExpression("color:[^;]+"), "color: #10B981")
                .replace(QRegularExpression("border:[^;]+"), "border: 1px solid #10B981"));
        m_controller->approveToolCall(call);
    } else {
        onGenerationStopped();
    }
}
