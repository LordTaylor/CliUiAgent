#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace CodeHex {

class DiffHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit DiffHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat m_addedFormat;
    QTextCharFormat m_removedFormat;
    QTextCharFormat m_headerFormat;
};

} // namespace CodeHex
