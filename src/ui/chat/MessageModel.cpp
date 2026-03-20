#include "MessageModel.h"
#include <QFileInfo>
#include <QVariant>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QModelIndex>
#include <QHash>
#include <QByteArray>
#include <QStringList>
#include "PrecomputedLayout.h"

namespace CodeHex {

MessageModel::MessageModel(QObject* parent) : QAbstractListModel(parent) {}

void MessageModel::setSession(Session* session) {
    beginResetModel();
    m_session = session;
    m_visible.clear();
    m_loadedOffset = 0;
    if (m_session) {
        // Load last PAGE_SIZE messages
        const int total = m_session->messages.size();
        const int start = qMax(0, total - PAGE_SIZE);
        m_visible = m_session->messages.mid(start);
        m_loadedOffset = total - start;
    }
    endResetModel();
}

void MessageModel::appendMessage(const Message& msg) {
    beginInsertRows({}, m_visible.size(), m_visible.size());
    m_visible.append(msg);
    precomputeLayout(m_visible.last());
    m_loadedOffset++;
    endInsertRows();
}

void MessageModel::updateLastMessage(const QString& text) {
    if (m_visible.isEmpty()) return;
    const int lastRow = m_visible.size() - 1;
    Message& lastMsg = m_visible[lastRow];

    if (!lastMsg.contentBlocks.isEmpty() && lastMsg.contentBlocks.first().type == BlockType::Text) {
        lastMsg.contentBlocks.first().content = text;
    } else {
        lastMsg.contentBlocks.clear();
        lastMsg.contentBlocks.append(CodeBlock{text, BlockType::Text});
        lastMsg.contentTypes.clear();
        lastMsg.contentTypes.append(Message::ContentType::Text);
    }

    precomputeLayout(lastMsg);

    const QModelIndex idx = index(lastRow);
    emit dataChanged(idx, idx, {TextRole, ContentBlocksRole, ContentTypesRole, RawMessageRole});
}

void MessageModel::setViewWidth(int width) {
    if (m_viewWidth == width) return;
    m_viewWidth = width;
    // Recompute all visible layouts if width changes significantly
    for (Message& msg : m_visible) {
        precomputeLayout(msg);
    }
    if (!m_visible.isEmpty()) {
        emit dataChanged(index(0), index(m_visible.size() - 1));
    }
}

void MessageModel::precomputeLayout(Message& msg) const {
    auto layout = std::make_shared<PrecomputedLayout>();
    layout->lastViewWidth = m_viewWidth;

    const int kAvatarSize = 32;
    const int kBubblePadding = 12;
    const int kMaxBubbleWidth = 520;
    const int kRowMargin = 8;
    const int kBadgeHeight = 20;

    const int maxW = qMin(kMaxBubbleWidth, m_viewWidth - 2 * kAvatarSize - 20);
    const int textW = maxW - 2 * kBubblePadding;

    int totalHeight = 0;

    for (const auto& block : msg.contentBlocks) {
        PrecomputedLayout::BlockLayout bl;
        bl.doc = std::make_shared<QTextDocument>();
        
        // Use a default font specifically for the document to match rendering
        QFont docFont("Inter", 13);
        bl.doc->setDefaultFont(docFont);
        bl.doc->setDocumentMargin(0);

        bool isMarkdown = (msg.role == Message::Role::Assistant && block.type == BlockType::Text) ||
                          (block.type != BlockType::Text);

        if (isMarkdown) {
            bl.doc->setMarkdown(block.content);
        } else {
            bl.doc->setPlainText(block.content);
        }

        // Set width constraint and force layout
        bl.doc->setTextWidth(textW);
        
        // Use idealWidth to get the actual minimum width needed for the text
        int contentWidth = static_cast<int>(std::ceil(bl.doc->idealWidth()));
        bl.width = qMax(60, qMin(maxW, contentWidth + 2 * kBubblePadding));
        
        // Re-set text width to the final bubble content width to get accurate height
        bl.doc->setTextWidth(bl.width - 2 * kBubblePadding);
        
        // Calculate height with specific header offsets
        int headerOffset = 0;
        if (block.type == BlockType::Output || block.type == BlockType::Thinking) {
            headerOffset = 24;
        } else if (block.type == BlockType::ToolCall) {
            headerOffset = 0; // ToolCall uses a fixed height below
        }

        int docHeight = static_cast<int>(bl.doc->size().height());
        int blockH = docHeight + 2 * kBubblePadding + headerOffset;
        
        if (block.type == BlockType::ToolCall) {
            blockH = 40; // Override for compact tool call
            bl.width = qMin(maxW, 200); // Fixed roughly
        }
        
        bl.height = blockH;
        layout->blocks.append(bl);
        totalHeight += blockH + kRowMargin;
    }

    if (!msg.attachments.isEmpty()) {
        totalHeight += kBadgeHeight + 4; // kBadgeMargin = 4
    }

    layout->totalHeight = qMax(totalHeight, kAvatarSize + 2 * kRowMargin);
    msg.layoutCache = layout;
}


void MessageModel::loadMoreMessages() {
    if (!canLoadMore()) return;
    const int total = m_session->messages.size();
    const int alreadyLoaded = m_loadedOffset;
    const int remaining = total - alreadyLoaded;
    const int toLoad = qMin(PAGE_SIZE, remaining);
    const int start = remaining - toLoad;

    const QList<Message> newMsgs = m_session->messages.mid(start, toLoad);

    beginInsertRows({}, 0, toLoad - 1);
    for (int i = newMsgs.size() - 1; i >= 0; --i)
        m_visible.prepend(newMsgs[i]);
    m_loadedOffset += toLoad;
    endInsertRows();
}

bool MessageModel::canLoadMore() const {
    if (!m_session) return false;
    return m_loadedOffset < m_session->messages.size();
}

void MessageModel::clear() {
    beginResetModel();
    m_visible.clear();
    m_session = nullptr;
    m_loadedOffset = 0;
    endResetModel();
}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_visible.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_visible.size()) return {};
    const Message& msg = m_visible[index.row()];

    switch (role) {
        case Qt::DisplayRole:
        case TextRole:        return msg.textFromContentBlocks();
        case RoleRole:        return static_cast<int>(msg.role);
        case ContentBlocksRole: return QVariant::fromValue(msg.contentBlocks); // New
        case ContentTypesRole:  return QVariant::fromValue(msg.contentTypes);  // New
        case TimestampRole:   return msg.timestamp;
        case TokenCountRole:  return msg.tokenCount;
        case AttachmentsRole: {
            // Return a QStringList of "type:filename" for each attachment so the
            // delegate can render a compact badge without knowing Attachment details.
            QStringList badge;
            for (const Attachment& a : msg.attachments) {
                const QString icon = (a.type == Attachment::Type::Image)  ? "🖼" :
                                     (a.type == Attachment::Type::Audio)  ? "🎤" : "📄";
                badge << icon + " " + QFileInfo(a.filePath).fileName();
            }
            return badge;
        }
        case RawMessageRole: return QVariant::fromValue(msg); // New
        default: return {};
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    return {
        {TextRole,        "text"},
        {RoleRole,        "role"},
        {TimestampRole,   "timestamp"},
        {TokenCountRole,  "tokenCount"},
        {AttachmentsRole, "attachments"},
        {ContentBlocksRole, "contentBlocks"}, // New
        {ContentTypesRole, "contentTypes"},   // New
        {RawMessageRole,  "rawMessage"},     // New
    };
}

}  // namespace CodeHex