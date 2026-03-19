#pragma once
#include <QAbstractListModel>
#include "../../data/Message.h"
#include "../../data/Session.h"

namespace CodeHex {

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        TextRole = Qt::UserRole,
        RoleRole,
        ContentTypeRole,
        TimestampRole,
        FilePathRole,
        TokenCountRole,
        AttachmentsRole,
    };

    explicit MessageModel(QObject* parent = nullptr);

    void setSession(Session* session);
    void appendMessage(const Message& msg);
    void loadMoreMessages();
    bool canLoadMore() const;
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    static constexpr int PAGE_SIZE = 10;

    Session* m_session = nullptr;
    QList<Message> m_visible;
    int m_loadedOffset = 0;  // index from the END of session->messages
};

}  // namespace CodeHex
