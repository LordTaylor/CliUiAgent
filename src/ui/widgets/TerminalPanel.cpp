#include "TerminalPanel.h"
#include <QVBoxLayout>
#include <QPropertyAnimation>

namespace CodeHex {

TerminalPanel::TerminalPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_toggleBtn = new QPushButton("▶ Terminal", this);
    m_toggleBtn->setObjectName("terminalToggleBtn");
    m_toggleBtn->setCheckable(true);
    m_toggleBtn->setFixedHeight(kCollapsedHeight);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_toggleBtn);

    m_terminal = new TerminalWidget(this);
    m_terminal->setMaximumHeight(0);
    layout->addWidget(m_terminal);

    m_anim = new QPropertyAnimation(m_terminal, "maximumHeight", this);
    m_anim->setDuration(200);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_toggleBtn, &QPushButton::toggled, this, &TerminalPanel::setExpanded);
    
    // Initial style (Glassmorphism for the button)
    m_toggleBtn->setStyleSheet(
        "QPushButton { background: rgba(30,30,30,0.8); border: none; border-top: 1px solid rgba(255,255,255,0.1); color: #94A3B8; text-align: left; padding-left: 10px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(40,40,40,0.9); color: #F1F5F9; }"
        "QPushButton:checked { color: #60A5FA; }"
    );
}

void TerminalPanel::appendOutput(const QString& text) {
    if (!m_expanded && !text.trimmed().isEmpty()) {
        setExpanded(true);
    }
    m_terminal->appendOutput(text);
}

void TerminalPanel::appendError(const QString& text) {
    if (!m_expanded && !text.trimmed().isEmpty()) {
        setExpanded(true);
    }
    m_terminal->appendError(text);
}

void TerminalPanel::clear() {
    m_terminal->clearTerminal();
}

void TerminalPanel::setExpanded(bool expanded) {
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    m_toggleBtn->setChecked(expanded);
    m_toggleBtn->setText(expanded ? "▼ Terminal" : "▶ Terminal");

    m_anim->stop();
    m_anim->setStartValue(m_terminal->maximumHeight());
    m_anim->setEndValue(expanded ? kExpandedHeight : 0);
    m_anim->start();
}

void TerminalPanel::toggleExpanded() {
    setExpanded(!m_expanded);
}

} // namespace CodeHex
