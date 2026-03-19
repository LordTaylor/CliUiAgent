#include "ChatView.h"
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

void ChatView::scrollContentsBy(int dx, int dy) {
    QListView::scrollContentsBy(dx, dy);
    // Trigger lazy-load when scrolled to top
    if (m_msgModel && verticalScrollBar()->value() == 0 && m_msgModel->canLoadMore()) {
        const int prevCount = m_msgModel->rowCount();
        emit loadMoreRequested();
        // Restore scroll position so user doesn't jump
        const int newCount = m_msgModel->rowCount();
        const int added = newCount - prevCount;
        if (added > 0) {
            // Approximate: scroll down by added rows
            verticalScrollBar()->setValue(verticalScrollBar()->singleStep() * added * 50);
        }
    }
}

}  // namespace CodeHex
