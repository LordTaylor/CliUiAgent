#include "DiffWidget.h"
#include <QFontDatabase>
#include <QVBoxLayout>

namespace CodeHex {

DiffWidget::DiffWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_explanationLabel = new QLabel(this);
    m_explanationLabel->setWordWrap(true);
    m_explanationLabel->setStyleSheet("color: #9CA3AF; font-style: italic; padding: 4px;");
    layout->addWidget(m_explanationLabel);

    m_diffEdit = new QPlainTextEdit(this);
    m_diffEdit->setReadOnly(true);
    m_diffEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    m_diffEdit->setFont(monoFont);
    
    m_diffEdit->setStyleSheet(
        "background-color: #111827; "
        "color: #E5E7EB; "
        "border: 1px solid #374151; "
        "border-radius: 6px; "
    );

    m_highlighter = new DiffHighlighter(m_diffEdit->document());
    layout->addWidget(m_diffEdit);
}

void DiffWidget::setDiff(const QString& diffText) {
    m_diffEdit->setPlainText(diffText);
}

void DiffWidget::setExplanation(const QString& explanation) {
    if (explanation.isEmpty()) {
        m_explanationLabel->hide();
    } else {
        m_explanationLabel->setText("💡 <b>AI Explanation:</b> " + explanation);
        m_explanationLabel->show();
    }
}

} // namespace CodeHex
