#pragma once
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QWidget>

namespace CodeHex {

class ConsoleWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConsoleWidget(QWidget* parent = nullptr);

    void appendText(const QString& text);
    void clear();

public slots:
    void toggleExpanded();
    void setExpanded(bool expanded);

private:
    QPlainTextEdit* m_edit;
    QPushButton* m_toggleBtn;
    QPropertyAnimation* m_anim;
    bool m_expanded = false;

    static constexpr int kCollapsedHeight = 24;
    static constexpr int kExpandedHeight  = 200;
};

}  // namespace CodeHex
