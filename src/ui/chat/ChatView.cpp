#include "ChatView.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QScrollBar>
#include "MessageDelegate.h"
#include "MessageModel.h"

namespace CodeHex {

ChatView::ChatView(QWidget* parent) : QListView(parent) {
    setItemDelegate(new MessageDelegate(this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setSpacing(4);
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void ChatView::setMessageModel(MessageModel* model) {
    m_msgModel = model;
    setModel(model);
}

void ChatView::scrollToBottom() {
    if (model() && model()->rowCount() > 0) {
        scrollTo(model()->index(model()->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
    }
}

void ChatView::contextMenuEvent(QContextMenuEvent* event) {
    const QModelIndex idx = indexAt(event->pos());
    if (!idx.isValid()) return;

    const QString text = idx.data(MessageModel::TextRole).toString();
    if (text.isEmpty()) return;

    QMenu menu(this);
    QAction* copyAct = menu.addAction("Copy message");
    copyAct->setShortcut(QKeySequence::Copy);

    if (menu.exec(event->globalPos()) == copyAct) {
        QApplication::clipboard()->setText(text);
    }
}

void ChatView::scrollContentsBy(int dx, int dy) {
    QListView::scrollContentsBy(dx, dy);
    // Trigger lazy-load when scrolled to top
    if (m_msgModel && verticalScrollBar()->value() == 0 && m_msgModel->canLoadMore()) {
        const int prevCount = m_msgModel->rowCount();
        const QPersistentModelIndex prevTopIndex = m_msgModel->index(0, 0);

        emit loadMoreRequested();

        // Restore scroll position so user doesn't jump
        if (m_msgModel->rowCount() > prevCount && prevTopIndex.isValid()) {
            scrollTo(prevTopIndex, QAbstractItemView::PositionAtTop);
        }
    }
}

}  // namespace CodeHex
