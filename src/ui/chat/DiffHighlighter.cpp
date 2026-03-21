#include "DiffHighlighter.h"

namespace CodeHex {

DiffHighlighter::DiffHighlighter(QTextDocument* parent) 
    : QSyntaxHighlighter(parent) {
    
    m_addedFormat.setForeground(QColor("#10B981")); // Green
    m_addedFormat.setBackground(QColor("#064E3B"));
    
    m_removedFormat.setForeground(QColor("#EF4444")); // Red
    m_removedFormat.setBackground(QColor("#7F1D1D"));
    
    m_headerFormat.setForeground(QColor("#3B82F6")); // Blue
    m_headerFormat.setFontWeight(QFont::Bold);
}

void DiffHighlighter::highlightBlock(const QString& text) {
    if (text.startsWith("+")) {
        setFormat(0, text.length(), m_addedFormat);
    } else if (text.startsWith("-")) {
        setFormat(0, text.length(), m_removedFormat);
    } else if (text.startsWith("@@") || text.startsWith("---") || text.startsWith("+++")) {
        setFormat(0, text.length(), m_headerFormat);
    }
}

} // namespace CodeHex
