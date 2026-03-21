#include "SessionPanel.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "../../core/SessionManager.h"
#include "../../data/Session.h"

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

    // Single-click → navigate to session; double-click → rename inline
    connect(m_list, &QListWidget::itemClicked,
            this, &SessionPanel::onItemClicked);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &SessionPanel::onItemDoubleClicked);
    connect(m_list, &QListWidget::itemChanged,
            this, &SessionPanel::onItemChanged);
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

void SessionPanel::selectSession(const QString& sessionId) {
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toString() == sessionId) {
            m_list->setCurrentRow(i);
            break;
        }
    }
}

void SessionPanel::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    emit sessionSelected(item->data(Qt::UserRole).toString());
}

void SessionPanel::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    // Enable editing for this item so the user can rename the session inline.
    // The rename is committed in onItemChanged().
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_list->editItem(item);
}

void SessionPanel::onItemChanged(QListWidgetItem* item) {
    if (!item) return;
    const QString id = item->data(Qt::UserRole).toString();
    const QString newTitle = item->text().trimmed();
    if (newTitle.isEmpty()) return;

    Session* s = m_manager->openSession(id);
    if (!s) return;
    s->title = newTitle;
    s->save();

    // Remove editable flag after rename so accidental clicks don't re-enter edit mode
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
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
