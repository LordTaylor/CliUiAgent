#include "TerminalWidget.h"
#include <QScrollBar>
#include <QKeyEvent>
#include <QRegularExpression>

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
    
    // Simple ANSI state machine
    static const QRegularExpression ansiRe("\x1b\\[([0-9;]*)m");
    int lastPos = 0;
    auto it = ansiRe.globalMatch(text);
    
    while (it.hasNext()) {
        auto match = it.next();
        // Insert text before the sequence
        insertPlainText(text.mid(lastPos, match.capturedStart() - lastPos));
        
        // Apply sequence
        applyAnsiSequence(match.captured(1));
        lastPos = match.capturedEnd();
    }
    
    // Insert remaining text
    insertPlainText(text.mid(lastPos));
    
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::applyAnsiSequence(const QString& sequence) {
    QTextCharFormat fmt = currentCharFormat();
    QStringList codes = sequence.split(';');
    
    if (sequence.isEmpty() || codes.contains("0")) {
        // Reset
        fmt = QTextCharFormat();
    } else {
        for (const QString& codeStr : codes) {
            int code = codeStr.toInt();
            if (code == 1) fmt.setFontWeight(QFont::Bold);
            else if (code >= 30 && code <= 37) {
                // Standard 8 colors
                static const QColor colors[] = {
                    Qt::black, Qt::red, Qt::green, Qt::yellow,
                    Qt::blue, Qt::magenta, Qt::cyan, Qt::white
                };
                fmt.setForeground(colors[code - 30]);
            }
        }
    }
    setCurrentCharFormat(fmt);
}

void TerminalWidget::appendError(const QString& text) {
    if (text.isEmpty()) return;
    moveCursor(QTextCursor::End);
    
    QTextCharFormat fmt;
    fmt.setForeground(QColor(239, 68, 68)); // Red-500
    setCurrentCharFormat(fmt);
    
    appendOutput(text); // Use appendOutput to handle any nested ANSI in error too
    
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
