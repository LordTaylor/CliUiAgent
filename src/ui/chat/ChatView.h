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

    void scrollToBottomSmooth();
    void setSearchTerm(const QString& term);

signals:
    void loadMoreRequested();
    void rerunRequested(const QString& text);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    MessageModel* m_msgModel = nullptr;
    bool m_autoScroll = true;
    QString m_searchTerm;
};

}  // namespace CodeHex
