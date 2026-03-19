#include "ConsoleWidget.h"
#include <QFont>
#include <QPushButton>
#include <QVBoxLayout>

namespace CodeHex {

ConsoleWidget::ConsoleWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_toggleBtn = new QPushButton("▶ Console", this);
    m_toggleBtn->setObjectName("consoleToggleBtn");
    m_toggleBtn->setCheckable(true);
    m_toggleBtn->setFixedHeight(kCollapsedHeight);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_toggleBtn);

    m_edit = new QPlainTextEdit(this);
    m_edit->setReadOnly(true);
    m_edit->setMaximumBlockCount(2000);
    m_edit->setObjectName("consoleEdit");
    QFont mono("Menlo", 11);
    mono.setStyleHint(QFont::Monospace);
    m_edit->setFont(mono);
    m_edit->setMaximumHeight(0);
    layout->addWidget(m_edit);

    m_anim = new QPropertyAnimation(m_edit, "maximumHeight", this);
    m_anim->setDuration(150);

    connect(m_toggleBtn, &QPushButton::toggled, this, &ConsoleWidget::setExpanded);
}

void ConsoleWidget::appendText(const QString& text) {
    m_edit->moveCursor(QTextCursor::End);
    m_edit->insertPlainText(text);
    m_edit->moveCursor(QTextCursor::End);
}

void ConsoleWidget::clear() {
    m_edit->clear();
}

void ConsoleWidget::toggleExpanded() {
    setExpanded(!m_expanded);
}

void ConsoleWidget::setExpanded(bool expanded) {
    m_expanded = expanded;
    m_toggleBtn->setChecked(expanded);
    m_toggleBtn->setText(expanded ? "▼ Console" : "▶ Console");

    m_anim->stop();
    m_anim->setStartValue(m_edit->maximumHeight());
    m_anim->setEndValue(expanded ? kExpandedHeight : 0);
    m_anim->start();
}

}  // namespace CodeHex
