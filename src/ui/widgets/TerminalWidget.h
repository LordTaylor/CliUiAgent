#pragma once
#include <QPlainTextEdit>

namespace CodeHex {

class TerminalWidget : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget* parent = nullptr);

    void appendOutput(const QString& text);
    void appendError(const QString& text);
    void clearTerminal();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupAppearance();
};

} // namespace CodeHex
