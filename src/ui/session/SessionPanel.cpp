#include "SessionPanel.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "../../core/SessionManager.h"

namespace CodeHex {

SessionPanel::SessionPanel(SessionManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_list = new QListWidget(this);
    m_list->setObjectName("sessionList");
    layout->addWidget(m_list);

    auto* btnRow = new QHBoxLayout;
    m_newBtn = new QPushButton("+ New", this);
    m_deleteBtn = new QPushButton("🗑", this);
    m_deleteBtn->setToolTip("Delete selected session");
    btnRow->addWidget(m_newBtn);
    btnRow->addWidget(m_deleteBtn);
    layout->addLayout(btnRow);

    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &SessionPanel::onItemDoubleClicked);
    connect(m_newBtn,    &QPushButton::clicked, this, &SessionPanel::onNewClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &SessionPanel::onDeleteClicked);

    connect(manager, &SessionManager::sessionsLoaded, this, &SessionPanel::refresh);
    connect(manager, &SessionManager::sessionCreated, this, [this](const QString&) { refresh(); });
    connect(manager, &SessionManager::sessionDeleted, this, [this](const QString&) { refresh(); });
}

void SessionPanel::refresh() {
    m_list->clear();
    for (const Session* s : m_manager->allSessions()) {
        auto* item = new QListWidgetItem(s->title);
        item->setData(Qt::UserRole, s->id.toString(QUuid::WithoutBraces));
        item->setToolTip(s->updatedAt.toLocalTime().toString("dd.MM.yyyy HH:mm"));
        m_list->addItem(item);
    }
}

void SessionPanel::onItemDoubleClicked() {
    const auto* item = m_list->currentItem();
    if (!item) return;
    emit sessionSelected(item->data(Qt::UserRole).toString());
}

void SessionPanel::onNewClicked() {
    emit newSessionRequested();
}

void SessionPanel::onDeleteClicked() {
    const auto* item = m_list->currentItem();
    if (!item) return;
    m_manager->deleteSession(item->data(Qt::UserRole).toString());
}

}  // namespace CodeHex
