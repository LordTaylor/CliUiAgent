#pragma once
#include <QStyledItemDelegate>
#include <QTextDocument>
#include "../../data/Message.h" // Include Message for the new paintMessageContent signature

namespace CodeHex {

class MessageDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit MessageDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    static constexpr int kBubblePadding  = 12;
    static constexpr int kBadgeHeight    = 20;
    static constexpr int kBadgeMargin    = 4;
    static constexpr int kBubbleRadius  = 10;
    static constexpr int kMaxBubbleWidth = 520;
    static constexpr int kAvatarSize    = 32;
    static constexpr int kRowMargin     = 8;

    // New: Generic method to paint message content blocks.
    void paintMessageContent(QPainter* p, const QStyleOptionViewItem& opt,
                             const Message& msg) const;

    // isMarkdown=true: renders CommonMark (assistant); false: plain text (user)
    QTextDocument* makeTextDoc(const QString& text, int maxWidth, bool isMarkdown) const;
};

}  // namespace CodeHex