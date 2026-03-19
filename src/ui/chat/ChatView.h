#pragma once
#include <QListView>

namespace CodeHex {

class MessageModel;

class ChatView : public QListView {
    Q_OBJECT
public:
    explicit ChatView(QWidget* parent = nullptr);

    void setMessageModel(MessageModel* model);
    void scrollToBottom();

signals:
    void loadMoreRequested();

protected:
    void scrollContentsBy(int dx, int dy) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    MessageModel* m_msgModel = nullptr;
};

}  // namespace CodeHex
