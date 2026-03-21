#include "ChatView.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QScrollBar>
#include <QResizeEvent>
#include <QAbstractListModel>
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
    setSelectionMode(QAbstractItemView::SingleSelection); // Enabled selection for bubbles
    setFrameShape(QFrame::NoFrame);
    setSpacing(6); // Increased spacing slightly
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void ChatView::setMessageModel(MessageModel* model) {
    if (m_msgModel) {
        disconnect(m_msgModel, nullptr, this, nullptr);
    }
    m_msgModel = model;
    setModel(model);
    if (m_msgModel) {
        m_msgModel->setViewWidth(viewport()->width());
        
        connect(m_msgModel, &QAbstractListModel::rowsInserted, this, [this]() {
            if (m_autoScroll) scrollToBottomSmooth();
        });
        connect(m_msgModel, &QAbstractListModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QVector<int>&) {
            if (m_autoScroll) scrollToBottomSmooth();
        });
    }
}

bool ChatView::autoScrollEnabled() const { return m_autoScroll; }
void ChatView::setAutoScrollEnabled(bool enabled) { m_autoScroll = enabled; }

void ChatView::setSearchTerm(const QString& term) {
    if (m_searchTerm == term) return;
    m_searchTerm = term;
    if (m_msgModel) {
        m_msgModel->setSearchTerm(term);
    }
}

void ChatView::scrollToBottom() {
    scrollToBottomSmooth();
}

void ChatView::scrollToBottomSmooth() {
    if (!model() || model()->rowCount() == 0) return;
    
    QScrollBar* bar = verticalScrollBar();
    // If we are near the bottom (within 20% of viewport height), stay at the bottom
    int threshold = qMax(100, (int)(viewport()->height() * 0.2));
    bool atBottom = (bar->value() >= bar->maximum() - threshold);
    
    if (atBottom || bar->maximum() == 0) {
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
        } else {
            QAction* copyTextAct = menu.addAction("Copy message text");
            QAction* copyMdAct = menu.addAction("Copy as Markdown");
            QAction* selected = menu.exec(event->globalPos());
            
            if (selected == copyTextAct) {
                QApplication::clipboard()->setText(msg.textFromContentBlocks());
            } else if (selected == copyMdAct) {
                // If we had a toMarkdown() helper, we'd use it here. 
                // For now, content blocks already contain markdown if they are asst messages.
                QString full;
                for (const auto& b : msg.contentBlocks) full += b.content + "\n\n";
                QApplication::clipboard()->setText(full.trimmed());
            }
            return;
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
                if (msg.isInternal && delegate->isInternalChipClicked(event->pos(), visualRect(idx), msg)) {
                    m_msgModel->toggleInternalExpand(idx.row());
                    event->accept();
                    return;
                }
                if (delegate->isEyeButtonClicked(event->pos(), visualRect(idx), msg)) {
                    m_msgModel->toggleThinkingVisibility(idx.row());
                    event->accept();
                    return;
                }
                
                int copyOutIdx = delegate->copyOutputClicked(event->pos(), visualRect(idx), msg);
                if (copyOutIdx != -1) {
                    QApplication::clipboard()->setText(msg.contentBlocks[copyOutIdx].content);
                    event->accept();
                    return;
                }
                
                int rerunIdx = delegate->rerunClicked(event->pos(), visualRect(idx), msg);
                if (rerunIdx != -1) {
                    emit rerunRequested(msg.contentBlocks[rerunIdx].content);
                    event->accept();
                    return;
                }
                
                int copyIdx = delegate->copyBlockIndexAt(event->pos(), visualRect(idx), msg);
                if (copyIdx != -1) {
                    QApplication::clipboard()->setText(msg.contentBlocks[copyIdx].content);
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
