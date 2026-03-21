#pragma once
#include <QDialog>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "../../data/ToolCall.h"

namespace CodeHex {

class DiffWidget;

class ToolApprovalDialog : public QDialog {
    Q_OBJECT
public:
    explicit ToolApprovalDialog(const ToolCall& call, QWidget* parent = nullptr);

    bool approved() const { return m_approved; }

private slots:
    void onApprove();
    void onDeny();

private:
    ToolCall m_call;
    bool m_approved = false;

    QLabel* m_infoLabel;
    DiffWidget* m_diffWidget; // Rich diff view
    QPushButton* m_approveBtn;
    QPushButton* m_denyBtn;
};

} // namespace CodeHex
