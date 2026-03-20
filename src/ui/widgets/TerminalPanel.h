#pragma once
#include <QWidget>
#include <QPushButton>
#include <QPropertyAnimation>
#include "TerminalWidget.h"

namespace CodeHex {

class TerminalPanel : public QWidget {
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget* parent = nullptr);

public slots:
    void appendOutput(const QString& text);
    void appendError(const QString& text);
    void clear();
    void setExpanded(bool expanded);
    void toggleExpanded();

private:
    TerminalWidget* m_terminal;
    QPushButton* m_toggleBtn;
    QPropertyAnimation* m_anim;
    bool m_expanded = false;

    static constexpr int kCollapsedHeight = 28;
    static constexpr int kExpandedHeight  = 220;
};

} // namespace CodeHex
