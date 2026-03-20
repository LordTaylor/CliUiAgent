#include "PluginsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>

namespace CodeHex {

PluginsDialog::PluginsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Scripts & Plugins");
    resize(400, 400);
    setupUi();
}

void PluginsDialog::setScratchpadPath(const QString& path) {
    m_scratchpadPath = path;
    loadScripts();
}

void PluginsDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* header = new QLabel("<b>📜 Local Scripts (.agent/scratchpad)</b>", this);
    layout->addWidget(header);

    m_list = new QListWidget(this);
    m_list->setObjectName("pluginsList");
    layout->addWidget(m_list);

    auto* btnLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton("Refresh", this);
    connect(refreshBtn, &QPushButton::clicked, this, &PluginsDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);

    btnLayout->addStretch();
    
    auto* runBtn = new QPushButton("Run selected", this);
    runBtn->setStyleSheet("background-color: #059669; color: white;");
    connect(runBtn, &QPushButton::clicked, this, &PluginsDialog::onRunScript);
    btnLayout->addWidget(runBtn);

    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void PluginsDialog::loadScripts() {
    m_list->clear();
    QDir dir(m_scratchpadPath);
    if (!dir.exists()) return;

    QStringList filters;
    filters << "*.py" << "*.sh" << "*.js" << "*.lua";
    dir.setNameFilters(filters);

    QFileInfoList list = dir.entryInfoList();
    for (const QFileInfo& file : list) {
        auto* item = new QListWidgetItem(file.fileName(), m_list);
        item->setToolTip(file.absoluteFilePath());
        
        // Add a small icon based on extension
        if (file.suffix() == "py") item->setText("🐍 " + item->text());
        else if (file.suffix() == "sh") item->setText("🐚 " + item->text());
        else item->setText("📄 " + item->text());
    }
}

void PluginsDialog::onRefresh() {
    loadScripts();
}

void PluginsDialog::onRunScript() {
    auto* current = m_list->currentItem();
    if (!current) return;
    // Implementation for running scripts could pass the path to the TerminalPanel or a script runner.
}

} // namespace CodeHex
