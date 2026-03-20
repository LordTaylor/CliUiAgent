#include "ToolApprovalDialog.h"
#include <QJsonDocument>
#include <QHBoxLayout>
#include <QIcon>

namespace CodeHex {

ToolApprovalDialog::ToolApprovalDialog(const ToolCall& call, QWidget* parent)
    : QDialog(parent), m_call(call) {
    setWindowTitle("Tool Approval Required");
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    auto* warning = new QLabel("⚠️ <b>Safety Mode:</b> The agent requires your explicit approval to perform this action.", this);
    warning->setStyleSheet("color: #F59E0B;");
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto* title = new QLabel("<b>Request Details:</b>", this);
    layout->addWidget(title);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    
    QString details = "<b>Tool:</b> <code style='color: #10B981;'>" + call.name + "</code><br><br>";
    details += "<b>Arguments:</b><br>";
    details += "<pre style='font-size: 11px;'>" + QJsonDocument(call.input).toJson(QJsonDocument::Indented) + "</pre>";
    
    m_infoLabel->setText(details);
    m_infoLabel->setStyleSheet("background-color: #1F2937; color: #E5E7EB; padding: 12px; border: 1px solid #374151; border-radius: 8px;");
    layout->addWidget(m_infoLabel);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    
    m_denyBtn = new QPushButton("Deny", this);
    m_denyBtn->setObjectName("stopBtn"); // Use the red button style
    connect(m_denyBtn, &QPushButton::clicked, this, &ToolApprovalDialog::onDeny);
    btnLayout->addWidget(m_denyBtn);

    m_approveBtn = new QPushButton("Approve", this);
    m_approveBtn->setObjectName("sendBtn"); // Use the blue button style
    m_approveBtn->setDefault(true);
    connect(m_approveBtn, &QPushButton::clicked, this, &ToolApprovalDialog::onApprove);
    btnLayout->addWidget(m_approveBtn);

    layout->addLayout(btnLayout);

    // Apply dark theme styling to the dialog itself (since it's a separate window potentially)
    setStyleSheet("background-color: #111827; color: #F3F4F6;");
}

void ToolApprovalDialog::onApprove() {
    m_approved = true;
    accept();
}

void ToolApprovalDialog::onDeny() {
    m_approved = false;
    reject();
}

} // namespace CodeHex
