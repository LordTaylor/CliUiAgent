#pragma once
#include <QStyledItemDelegate>
#include <QTextDocument>

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
    static constexpr int kBubblePadding = 12;
    static constexpr int kBubbleRadius  = 10;
    static constexpr int kMaxBubbleWidth = 520;
    static constexpr int kAvatarSize    = 32;
    static constexpr int kRowMargin     = 8;

    void paintTextBubble(QPainter* p, const QStyleOptionViewItem& opt,
                         const QModelIndex& idx) const;
    void paintImageBubble(QPainter* p, const QStyleOptionViewItem& opt,
                          const QModelIndex& idx) const;
    void paintVoiceBubble(QPainter* p, const QStyleOptionViewItem& opt,
                          const QModelIndex& idx) const;

    QTextDocument* makeTextDoc(const QString& text, int maxWidth) const;
};

}  // namespace CodeHex
