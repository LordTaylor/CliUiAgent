#include "ChatView.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QScrollBar>
#include <QResizeEvent>
#include "MessageDelegate.h"
#include "MessageModel.h"
#include <QMouseEvent>
#include <QDebug>

namespace CodeHex {

ChatView::ChatView(QWidget* parent) : QListView(parent) {
    setItemDelegate(new MessageDelegate(this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformItemSizes(false);
    setResizeMode(QListView::Adjust);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setSpacing(4);
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void ChatView::setMessageModel(MessageModel* model) {
    m_msgModel = model;
    setModel(model);
    if (m_msgModel) {
        m_msgModel->setViewWidth(viewport()->width());
    }
}

bool ChatView::autoScrollEnabled() const { return m_autoScroll; }
void ChatView::setAutoScrollEnabled(bool enabled) { m_autoScroll = enabled; }

void ChatView::scrollToBottom() {
    scrollToBottomSmooth();
}

void ChatView::scrollToBottomSmooth() {
    if (!model() || model()->rowCount() == 0) return;
    
    QScrollBar* bar = verticalScrollBar();
    // If we are near the bottom (within 50px), stay at the bottom
    bool atBottom = (bar->value() >= bar->maximum() - 50);
    
    if (atBottom || m_autoScroll) {
        bar->setValue(bar->maximum());
    }
}

void ChatView::resizeEvent(QResizeEvent* event) {
    QListView::resizeEvent(event);
    if (m_msgModel) {
        m_msgModel->setViewWidth(viewport()->width());
    }
    if (m_autoScroll) {
        scrollToBottom();
    }
}

void ChatView::contextMenuEvent(QContextMenuEvent* event) {
    const QModelIndex idx = indexAt(event->pos());
    if (!idx.isValid()) return;

    Message msg = idx.data(MessageModel::RawMessageRole).value<Message>();
    auto* delegate = qobject_cast<MessageDelegate*>(itemDelegate());
    int blockIdx = -1;
    if (delegate) {
        blockIdx = delegate->blockIndexAt(event->pos(), visualRect(idx), msg);
    }

    QMenu menu(this);
    if (blockIdx != -1 && (msg.contentBlocks[blockIdx].type != BlockType::Text && 
                          msg.contentBlocks[blockIdx].type != BlockType::Thinking)) {
        QAction* copyCodeAct = menu.addAction("Copy code/output");
        if (menu.exec(event->globalPos()) == copyCodeAct) {
            QApplication::clipboard()->setText(msg.contentBlocks[blockIdx].content);
        }
    } else {
        const QString text = msg.textFromContentBlocks();
        
        if (msg.role == Message::Role::Assistant) {
            bool hasThinking = false;
            for (const auto& b : msg.contentBlocks) if (b.type == BlockType::Thinking) { hasThinking = true; break; }
            
            if (hasThinking) {
                QAction* toggleThinking = menu.addAction(msg.showThinking ? "Hide Reasoning" : "Show Reasoning");
                if (menu.exec(event->globalPos()) == toggleThinking) {
                    m_msgModel->toggleThinkingVisibility(idx.row());
                    return;
                }
            }
        }

        QAction* copyAct = menu.addAction("Copy entire message");
        if (menu.exec(event->globalPos()) == copyAct) {
            QApplication::clipboard()->setText(text);
        }
    }
}

void ChatView::mousePressEvent(QMouseEvent* event) {
    if (m_msgModel && event->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(event->pos());
        if (idx.isValid()) {
            Message msg = idx.data(MessageModel::RawMessageRole).value<Message>();
            auto* delegate = qobject_cast<MessageDelegate*>(itemDelegate());
            if (delegate) {
                if (delegate->isEyeButtonClicked(event->pos(), visualRect(idx), msg)) {
                    m_msgModel->toggleThinkingVisibility(idx.row());
                    event->accept();
                    return;
                }
                int blockIdx = delegate->blockIndexAt(event->pos(), visualRect(idx), msg);
                if (blockIdx != -1) {
                    m_msgModel->toggleBlock(idx.row(), blockIdx);
                    event->accept();
                    return;
                }
            }
        }
    }
    QListView::mousePressEvent(event);
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
