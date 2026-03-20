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

    bool autoScrollEnabled() const;
    void setAutoScrollEnabled(bool enabled);

signals:
    void loadMoreRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    MessageModel* m_msgModel = nullptr;
    bool m_autoScroll = true;
};

}  // namespace CodeHex
