#include "ToolApprovalDialog.h"
#include <QJsonDocument>
#include <QHBoxLayout>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include "DiffWidget.h"
#include "../../core/DiffUtils.h"

namespace CodeHex {

ToolApprovalDialog::ToolApprovalDialog(const ToolCall& call, QWidget* parent)
    : QDialog(parent), m_call(call) {
    setWindowTitle("Tool Approval Required");
    setMinimumSize(600, 450);

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
    
    QString details = "<b>Tool:</b> <code style='color: #10B981;'>" + call.name + "</code>";
    m_infoLabel->setText(details);
    layout->addWidget(m_infoLabel);

    m_diffWidget = new DiffWidget(this);
    m_diffWidget->setExplanation(call.explanation);

    bool showDiff = false;
    if (call.name == "replace_file_content") {
        QString target = call.input["TargetContent"].toString();
        QString replacement = call.input["ReplacementContent"].toString();
        auto diff = DiffUtils::generateDiff(target, replacement);
        m_diffWidget->setDiff(DiffUtils::toUnifiedString(diff));
        showDiff = true;
    } else if (call.name == "write_file_content" || call.name == "write_to_file") {
         QString path = call.input["TargetFile"].toString();
         QString newContent = call.input["CodeContent"].toString();
         QString oldContent;
         QFile file(path);
         if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
             oldContent = QTextStream(&file).readAll();
         }
         auto diff = DiffUtils::generateDiff(oldContent, newContent);
         m_diffWidget->setDiff(DiffUtils::toUnifiedString(diff));
         showDiff = true;
    }

    if (showDiff) {
        layout->addWidget(m_diffWidget);
    } else {
        auto* jsonLabel = new QLabel(this);
        jsonLabel->setWordWrap(true);
        jsonLabel->setText("<pre style='font-size: 11px;'>" + QJsonDocument(call.input).toJson(QJsonDocument::Indented) + "</pre>");
        jsonLabel->setStyleSheet("background-color: #1F2937; color: #E5E7EB; padding: 12px; border: 1px solid #374151; border-radius: 8px;");
        layout->addWidget(jsonLabel);
    }

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
