#include "ChatControlBanner.h"
#include <QHBoxLayout>
#include <QTimer>
#include <QVariant>

namespace CodeHex {

ChatControlBanner::ChatControlBanner(QWidget* parent) : QWidget(parent) {
    setObjectName("chatBanner");
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 4, 16, 4);
    layout->setSpacing(12);

    m_modelLabel = new QLabel("🤖 Agent Standby", this);
    m_modelLabel->setStyleSheet("color: #FBBF24; font-weight: 600; font-size: 11px;");
    layout->addWidget(m_modelLabel);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #D97706; font-weight: 500; font-size: 10px; margin-left: 10px;");
    m_statusLabel->setVisible(false);
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    m_autoApproveCheck = new QCheckBox("Auto-Approve", this);
    m_autoApproveCheck->setCursor(Qt::PointingHandCursor);
    m_autoApproveCheck->setStyleSheet(R"(
        QCheckBox { color: #B4B4B4; font-size: 11px; font-weight: 500; spacing: 4px; }
        QCheckBox::indicator { width: 14px; height: 14px; border-radius: 4px; border: 1px solid #45362B; background: #1A120A; }
        QCheckBox::indicator:checked { background-color: #D97706; border-color: #F59E0B; }
    )");
    layout->addWidget(m_autoApproveCheck);

    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setStyleSheet(R"(
        QPushButton { 
            background: rgba(217, 119, 6, 0.1); color: #FBBF24; border: 1px solid rgba(217, 119, 6, 0.3); 
            border-radius: 6px; padding: 2px 12px; font-size: 10px; font-weight: 700;
        }
        QPushButton:hover { background: rgba(217, 119, 6, 0.2); border-color: #D97706; }
    )");
    layout->addWidget(m_clearBtn);

    m_pulseTimer = new QTimer(this);
    m_pulseTimer->setInterval(50);
    connect(m_pulseTimer, &QTimer::timeout, this, &ChatControlBanner::onPulse);

    connect(m_autoApproveCheck, &QCheckBox::toggled, this, &ChatControlBanner::autoApproveChanged);
    connect(m_clearBtn, &QPushButton::clicked, this, &ChatControlBanner::clearChatRequested);

    setFixedHeight(36);
}

void ChatControlBanner::setThinking(bool thinking) {
    if (thinking) {
        m_pulseTimer->start();
        m_modelLabel->setText("🌀 Agent Thinking...");
    } else {
        m_pulseTimer->stop();
        m_pulseAlpha = 255;
        m_modelLabel->setStyleSheet("color: #FBBF24; font-weight: 600; font-size: 11px;");
        m_modelLabel->setText("🤖 Agent Ready");
        m_statusLabel->setVisible(false);
        m_statusLabel->setText("");
    }
}

void ChatControlBanner::setStatusText(const QString& text) {
    m_statusLabel->setText(text);
    m_statusLabel->setVisible(!text.isEmpty());
}

void ChatControlBanner::onPulse() {
    if (m_pulseDir) {
        m_pulseAlpha -= 15;
        if (m_pulseAlpha <= 100) m_pulseDir = false;
    } else {
        m_pulseAlpha += 15;
        if (m_pulseAlpha >= 255) m_pulseDir = true;
    }
    m_modelLabel->setStyleSheet(QString("color: rgba(16, 185, 129, %1/255.0); font-weight: 700; font-size: 11px;").arg(m_pulseAlpha/255.0));
}

void ChatControlBanner::setAutoApprove(bool enabled) {
    m_autoApproveCheck->setChecked(enabled);
}

bool ChatControlBanner::autoApprove() const {
    return m_autoApproveCheck->isChecked();
}

void ChatControlBanner::setModelName(const QString& name) {
    m_modelLabel->setText("🤖 Model: " + name);
}

} // namespace CodeHex
