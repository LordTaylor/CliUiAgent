#include "MessageModel.h"

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
        case TextRole:        return msg.text;
        case RoleRole:        return static_cast<int>(msg.role);
        case ContentTypeRole: return static_cast<int>(msg.contentType);
        case TimestampRole:   return msg.timestamp;
        case FilePathRole:    return msg.filePath;
        case TokenCountRole:  return msg.tokenCount;
        default: return {};
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    return {
        {TextRole,        "text"},
        {RoleRole,        "role"},
        {ContentTypeRole, "contentType"},
        {TimestampRole,   "timestamp"},
        {FilePathRole,    "filePath"},
        {TokenCountRole,  "tokenCount"},
    };
}

}  // namespace CodeHex
