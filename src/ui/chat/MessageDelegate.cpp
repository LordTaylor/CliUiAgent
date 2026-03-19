#include "MessageDelegate.h"
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPixmap>
#include <QTextDocument>
#include "../chat/MessageModel.h"
#include "../../data/Message.h"

namespace CodeHex {

MessageDelegate::MessageDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void MessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
    painter->save();
    const int type = index.data(MessageModel::ContentTypeRole).toInt();
    switch (static_cast<Message::ContentType>(type)) {
        case Message::ContentType::Image: paintImageBubble(painter, option, index); break;
        case Message::ContentType::Voice: paintVoiceBubble(painter, option, index); break;
        default:                          paintTextBubble(painter, option, index);  break;
    }
    painter->restore();
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    const QString text = index.data(MessageModel::TextRole).toString();
    const int viewWidth = option.rect.width() > 0 ? option.rect.width() : 600;
    const int maxW = qMin(kMaxBubbleWidth, viewWidth - 2 * kAvatarSize - 20);

    QTextDocument doc;
    doc.setDefaultFont(option.font);
    doc.setPlainText(text);
    doc.setTextWidth(maxW - 2 * kBubblePadding);
    const int h = static_cast<int>(doc.size().height()) + 2 * kBubblePadding + 2 * kRowMargin;
    return {viewWidth, qMax(h, kAvatarSize + 2 * kRowMargin)};
}

void MessageDelegate::paintTextBubble(QPainter* p, const QStyleOptionViewItem& opt,
                                      const QModelIndex& idx) const {
    const QString text = idx.data(MessageModel::TextRole).toString();
    const int role = idx.data(MessageModel::RoleRole).toInt();
    const bool isUser = (role == static_cast<int>(Message::Role::User));

    const QRect& r = opt.rect;
    const int viewWidth = r.width();
    const int maxW = qMin(kMaxBubbleWidth, viewWidth - 2 * kAvatarSize - 20);

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setPlainText(text);
    doc.setTextWidth(maxW - 2 * kBubblePadding);
    const int docH = static_cast<int>(doc.size().height());
    const int bubbleH = docH + 2 * kBubblePadding;
    const int bubbleW = qMin(maxW, static_cast<int>(doc.idealWidth()) + 2 * kBubblePadding);

    int x, y;
    y = r.top() + kRowMargin;
    if (isUser) {
        x = viewWidth - bubbleW - kAvatarSize - 12;
    } else {
        x = kAvatarSize + 12;
    }

    // Bubble background
    const QColor bubbleColor = isUser ? QColor(0x2563EB) : QColor(0x374151);
    p->setPen(Qt::NoPen);
    p->setBrush(bubbleColor);
    p->setRenderHint(QPainter::Antialiasing);
    p->drawRoundedRect(x, y, bubbleW, bubbleH, kBubbleRadius, kBubbleRadius);

    // Text
    p->save();
    p->translate(x + kBubblePadding, y + kBubblePadding);
    p->setPen(Qt::white);
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, Qt::white);
    doc.documentLayout()->draw(p, ctx);
    p->restore();
}

void MessageDelegate::paintImageBubble(QPainter* p, const QStyleOptionViewItem& opt,
                                       const QModelIndex& idx) const {
    const QString path = idx.data(MessageModel::FilePathRole).toString();
    const QRect& r = opt.rect;
    QPixmap pix(path);
    if (pix.isNull()) {
        p->drawText(r, Qt::AlignCenter, "[Image: " + path + "]");
        return;
    }
    const int maxDim = 200;
    pix = pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = r.left() + kAvatarSize + 12;
    const int y = r.top() + kRowMargin;
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    p->drawPixmap(x, y, pix);
}

void MessageDelegate::paintVoiceBubble(QPainter* p, const QStyleOptionViewItem& opt,
                                       const QModelIndex& idx) const {
    const QRect& r = opt.rect;
    const QColor bubbleColor(0x374151);
    p->setPen(Qt::NoPen);
    p->setBrush(bubbleColor);
    p->setRenderHint(QPainter::Antialiasing);
    const int bw = 160, bh = 40;
    const int x = r.left() + kAvatarSize + 12;
    const int y = r.top() + kRowMargin;
    p->drawRoundedRect(x, y, bw, bh, kBubbleRadius, kBubbleRadius);

    // Microphone icon placeholder
    p->setPen(Qt::white);
    p->drawText(QRect(x, y, bw, bh), Qt::AlignCenter, "🎤 Voice message");
}

}  // namespace CodeHex
