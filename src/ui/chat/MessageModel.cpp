#include "MessageModel.h"
#include <QFileInfo>
#include <QVariant> // For QVariant::fromValue

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
    if (m_session) {
        // session already has msg appended by ChatController
        // just update the visible list
    }
    beginInsertRows({}, m_visible.size(), m_visible.size());
    m_visible.append(msg);
    m_loadedOffset++;
    endInsertRows();
}

void MessageModel::updateLastMessage(const QString& text) {
    if (m_visible.isEmpty()) return;
    const int lastRow = m_visible.size() - 1;
    // Assuming the last message always has at least one text block and we're updating it
    if (!m_visible[lastRow].contentBlocks.isEmpty() && m_visible[lastRow].contentBlocks.first().type == BlockType::Text) {
        m_visible[lastRow].contentBlocks.first().content = text;
    } else {
        // If no text block exists, create one (e.g., initial streaming response)
        m_visible[lastRow].contentBlocks.clear(); // Clear existing content if types are mixed
        m_visible[lastRow].contentBlocks.append(CodeBlock{text, BlockType::Text});
        m_visible[lastRow].contentTypes.clear(); // Clear existing content types if types are mixed
        m_visible[lastRow].contentTypes.append(Message::ContentType::Text);
    }
    const QModelIndex idx = index(lastRow);
    emit dataChanged(idx, idx, {TextRole, ContentBlocksRole, ContentTypesRole}); // Notify about changed content
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