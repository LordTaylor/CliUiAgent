#pragma once
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QCache>
#include <QMutex>
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

    // Cache for rendered documents to avoid re-parsing markdown on every paint
    struct CacheKey {
        QString text;
        int width;
        bool isMarkdown;

        bool operator==(const CacheKey& other) const {
            return width == other.width && isMarkdown == other.isMarkdown && text == other.text;
        }
    };
    
    // Using a simple QString key for QCache: "width|isMarkdown|text_hash"
    mutable QCache<QString, QTextDocument> m_docCache;
    mutable QMutex m_cacheMutex;
};

}  // namespace CodeHex