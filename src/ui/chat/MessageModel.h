#pragma once
#include <QAbstractListModel>
#include "../../data/Message.h"
#include "../../data/Session.h"
#include "../../data/CodeBlock.h" // Added for CodeBlock

namespace CodeHex {

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        TextRole = Qt::UserRole,
        RoleRole,
        // ContentTypeRole, // Removed, will get from RawMessageRole
        TimestampRole,
        TokenCountRole,
        AttachmentsRole,
        ContentBlocksRole,   // New role for QList<CodeBlock>
        ContentTypesRole,    // New role for QList<Message::ContentType>
        RawMessageRole,      // New role to pass the entire Message object
    };

    explicit MessageModel(QObject* parent = nullptr);

    void setSession(Session* session);
    void appendMessage(const Message& msg);
    void appendToken(const QString& token);
    // Update the text of the last visible message in-place (live streaming).
    void updateLastMessage(const QString& text);
    void loadMoreMessages();
    bool canLoadMore() const;
    void clear();
    void toggleBlock(int row, int blockIndex);
    void toggleThinkingVisibility(int row);

    // Optimization: Pre-calculate layouts
    void setSearchTerm(const QString& term);

    // Optimization: Pre-calculate layouts
    void setViewWidth(int width);
    void precomputeLayout(Message& msg) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    static constexpr int PAGE_SIZE = 10;

    Session* m_session = nullptr;
    QList<Message> m_visible;
    int m_loadedOffset = 0;  // index from the END of session->messages
    int m_viewWidth = 600;
    QString m_searchTerm;
};

}  // namespace CodeHex