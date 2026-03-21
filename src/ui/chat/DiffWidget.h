#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include "DiffHighlighter.h"

namespace CodeHex {

class DiffWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiffWidget(QWidget* parent = nullptr);

    void setDiff(const QString& diffText);
    void setExplanation(const QString& explanation);

private:
    QPlainTextEdit* m_diffEdit;
    QLabel* m_explanationLabel;
    DiffHighlighter* m_highlighter;
};

} // namespace CodeHex
