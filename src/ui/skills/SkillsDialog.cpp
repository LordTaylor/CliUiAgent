#include "SkillsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>

namespace CodeHex {

SkillsDialog::SkillsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Agent Skills & Capabilities");
    resize(400, 450);
    setupUi();
}

void SkillsDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* header = new QLabel("<b>Available Skills</b>", this);
    header->setStyleSheet("font-size: 16px; color: #A855F7;");
    layout->addWidget(header);

    m_list = new QListWidget(this);
    m_list->setObjectName("skillsList");
    m_list->setSpacing(4);
    layout->addWidget(m_list);

    // Initial set of skills based on CodeHex features
    addSkill("Code Search", "Deep indexing and semantic search across the codebase.", "🔍");
    addSkill("Multi-file Edit", "Perform complex refactors across multiple files simultaneously.", "✏️");
    addSkill("Terminal Access", "Execute shell commands and monitor build processes.", "🐚");
    addSkill("Web Research", "Browse the internet for documentation and latest tech stack updates.", "🌍");
    addSkill("Audio Control", "Record and playback voice instructions for hands-free coding.", "🎙️");
    addSkill("System Awareness", "Understand OS state, environment variables, and project structure.", "🧠");

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* okBtn = new QPushButton("Got it", this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);
    layout->addLayout(btnLayout);
}

void SkillsDialog::addSkill(const QString& name, const QString& desc, const QString& icon) {
    auto* item = new QListWidgetItem(m_list);
    
    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    auto* iconLbl = new QLabel(icon);
    iconLbl->setStyleSheet("font-size: 20px;");
    layout->addWidget(iconLbl);

    auto* textContainer = new QWidget();
    auto* textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto* nameLbl = new QLabel("<b>" + name + "</b>");
    nameLbl->setStyleSheet("font-size: 13px; color: #F3F4F6;");
    auto* descLbl = new QLabel(desc);
    descLbl->setWordWrap(true);
    descLbl->setStyleSheet("font-size: 11px; color: #9CA3AF;");

    textLayout->addWidget(nameLbl);
    textLayout->addWidget(descLbl);
    layout->addWidget(textContainer, 1);

    item->setSizeHint(container->sizeHint());
    m_list->addItem(item);
    m_list->setItemWidget(item, container);
}

} // namespace CodeHex
