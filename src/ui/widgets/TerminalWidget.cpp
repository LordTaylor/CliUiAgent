#include "TerminalWidget.h"
#include <QScrollBar>
#include <QKeyEvent>

namespace CodeHex {

TerminalWidget::TerminalWidget(QWidget* parent) : QPlainTextEdit(parent) {
    setupAppearance();
    setReadOnly(true);
    setUndoRedoEnabled(false);
}

void TerminalWidget::setupAppearance() {
    // Monospace font
    QFont font("JetBrains Mono", 10);
    if (font.exactMatch()) {
        setFont(font);
    } else {
        setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    }

    // Colors will be mainly handled via QSS later, 
    // but setting some defaults to ensure classic terminal look
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(20, 20, 20));
    p.setColor(QPalette::Text, QColor(240, 240, 240));
    setPalette(p);
}

void TerminalWidget::appendOutput(const QString& text) {
    if (text.isEmpty()) return;
    moveCursor(QTextCursor::End);
    insertPlainText(text);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::appendError(const QString& text) {
    if (text.isEmpty()) return;
    moveCursor(QTextCursor::End);
    
    QTextCharFormat fmt;
    fmt.setForeground(QColor(239, 68, 68)); // Red-500
    setCurrentCharFormat(fmt);
    
    insertPlainText(text);
    
    // Reset format
    setCurrentCharFormat(QTextCharFormat());
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::clearTerminal() {
    clear();
}

void TerminalWidget::keyPressEvent(QKeyEvent* event) {
    // For now, terminal is read-only for output logs, 
    // but in future we can add input handling here.
    QPlainTextEdit::keyPressEvent(event);
}

} // namespace CodeHex
