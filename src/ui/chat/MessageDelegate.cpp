#include "MessageDelegate.h"
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QColor>
#include <QPixmap>
#include <QTextDocument>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <memory>
#include <QFileInfo> // Added
#include "MessageModel.h"
#include "PrecomputedLayout.h"
#include "../../data/CodeBlock.h"
#include "../../data/Message.h"

namespace CodeHex {

MessageDelegate::MessageDelegate(QObject* parent) 
    : QStyledItemDelegate(parent) {}

// makeTextDoc REMOVED - Logic moved to MessageModel::precomputeLayout

// ── paintMessageContent ───────────────────────────────────────────────────────
// Generic method to paint message content blocks.
void MessageDelegate::paintMessageContent(QPainter* p, const QStyleOptionViewItem& opt,
                                          const Message& msg) const {
    const bool isUser = (msg.role == Message::Role::User);
    const QRect& r = opt.rect;
    const int viewWidth = r.width();
    const int maxW = qMin(kMaxBubbleWidth, viewWidth - 2 * kAvatarSize - 20);

    int currentY = r.top() + kRowMargin;

    // Avatar drawing (once per message)
    const int avatarX = isUser ? viewWidth - kAvatarSize - 4 : 4;
    const int avatarY = currentY;
    if (isUser) {
        p->setBrush(QColor(0x1E40AF));
        p->setPen(Qt::NoPen);
        p->drawEllipse(avatarX, avatarY, kAvatarSize, kAvatarSize);
        p->setPen(Qt::white);
        p->drawText(QRect(avatarX, avatarY, kAvatarSize, kAvatarSize), Qt::AlignCenter, "U");
    } else {
        QPixmap pix(":/resources/icons/app.png");
        if (!pix.isNull()) {
            p->drawPixmap(avatarX, avatarY, kAvatarSize, kAvatarSize, pix);
        } else {
            p->setBrush(QColor(0x10B981));
            p->setPen(Qt::NoPen);
            p->drawEllipse(avatarX, avatarY, kAvatarSize, kAvatarSize);
            p->setPen(Qt::white);
            p->drawText(QRect(avatarX, avatarY, kAvatarSize, kAvatarSize), Qt::AlignCenter, "A");
        }
    }

    if (!msg.layoutCache) return;
    const auto& layout = *msg.layoutCache;

    int currentBlockIdx = 0;
    for (const CodeBlock& block : msg.contentBlocks) {
        if (currentBlockIdx >= layout.blocks.size()) break;
        const auto& bl = layout.blocks[currentBlockIdx++];

        int x = isUser ? viewWidth - bl.width - kAvatarSize - 12 : kAvatarSize + 12;
        if (block.type != BlockType::Text) {
            x = kAvatarSize + 12; // Assistant non-text blocks always left
        }

        // --- Bubble background ---
        QColor bubbleColor;
        if (isUser) {
            bubbleColor = QColor(0x2563EB);
        } else {
            switch(block.type) {
                case BlockType::Text:     bubbleColor = QColor(0x374151); break;
                case BlockType::Bash:
                case BlockType::Python:
                case BlockType::Lua:      bubbleColor = QColor(0x1F2937); break;
                case BlockType::Thinking:  bubbleColor = QColor(45, 55, 72, 180); break;
                case BlockType::Output:    bubbleColor = QColor(0x111827); break;
                case BlockType::ToolCall:  bubbleColor = QColor(0x374151); break;
                default:                  bubbleColor = QColor(0x374151); break;
            }
        }

        p->setPen(Qt::NoPen);
        p->setBrush(bubbleColor);
        p->setRenderHint(QPainter::Antialiasing);
        p->drawRoundedRect(x, currentY, bl.width, bl.height, kBubbleRadius, kBubbleRadius);

        // --- Text/Content ---
        int headerHeight = 0;
        if (!isUser) {
            if (block.type == BlockType::Thinking) {
                headerHeight = 24;
                p->setPen(QColor(0xA0AEC0));
                QFont headerFont = opt.font;
                headerFont.setItalic(true);
                headerFont.setPointSizeF(headerFont.pointSizeF() * 0.85);
                p->setFont(headerFont);
                p->drawText(QRect(x + kBubblePadding, currentY + 4, bl.width - 2 * kBubblePadding, headerHeight), 
                            Qt::AlignVCenter | Qt::AlignLeft, "💭 Agent thinking...");
                p->setFont(opt.font);
            } else if (block.type == BlockType::Output) {
                headerHeight = 24;
                p->setPen(QColor(0x10B981));
                QFont headerFont = opt.font;
                headerFont.setBold(true);
                headerFont.setPointSizeF(headerFont.pointSizeF() * 0.9);
                p->setFont(headerFont);
                p->drawText(QRect(x + kBubblePadding, currentY + 6, bl.width - 2 * kBubblePadding, headerHeight), 
                            Qt::AlignVCenter | Qt::AlignLeft, "✅ Tool Result");
                p->setFont(opt.font);
            } else if (block.type == BlockType::ToolCall) {
                p->setPen(QColor(0x60A5FA));
                p->drawText(QRect(x + kBubblePadding, currentY, bl.width - 2 * kBubblePadding, bl.height),
                            Qt::AlignCenter | Qt::AlignLeft, "⚙  " + block.content);
            }
        }

        if (block.type != BlockType::ToolCall) {
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding + headerHeight);
            QAbstractTextDocumentLayout::PaintContext ctx;
            
            QColor textColor = Qt::white;
            if (!isUser) {
                if (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua)
                    textColor = QColor(0x9CA3AF);
                else if (block.type == BlockType::Thinking)
                    textColor = QColor(0xCBD5E0);
                else if (block.type == BlockType::Output)
                    textColor = QColor(0xD1D5DB);
            }
            
            ctx.palette.setColor(QPalette::Text, textColor);
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();
        }

        currentY += bl.height + kRowMargin;
    }

    // Attachment badge - only for user messages with files
    // Assuming attachments are now handled directly from msg.attachments
    if (!msg.attachments.isEmpty() && isUser) {
        const int badgeY = currentY;
        QStringList badgeTexts;
        for (const auto& att : msg.attachments) {
            const QString icon = (att.type == Attachment::Type::Image)  ? "🖼" :
                                 (att.type == Attachment::Type::Audio)  ? "🎤" : "📄";
            badgeTexts << icon + " " + QFileInfo(att.filePath).fileName();
        }
        const QString badgeText = badgeTexts.join("  ");

        QFont badgeFont = opt.font;
        badgeFont.setPointSizeF(badgeFont.pointSizeF() * 0.82);
        p->setFont(badgeFont);

        // Pill background
        const QFontMetrics fm(badgeFont);
        const int textW = fm.horizontalAdvance(badgeText) + 16;
        const int badgeX = isUser ? viewWidth - textW - kAvatarSize - 12 : kAvatarSize + 12;

        p->setPen(Qt::NoPen);
        p->setBrush(QColor(0x1F2937));
        p->drawRoundedRect(badgeX, badgeY, textW, kBadgeHeight, 6, 6);

        p->setPen(QColor(0x9CA3AF));
        p->drawText(QRect(badgeX + 8, badgeY, textW - 8, kBadgeHeight),
                    Qt::AlignVCenter | Qt::AlignLeft, badgeText);

        p->setFont(opt.font);  // restore
    }
}


void MessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
    painter->save();
    const Message msg = index.data(MessageModel::RawMessageRole).value<Message>();

    if (!msg.contentBlocks.isEmpty() || !msg.attachments.isEmpty()) { // Also paint if only attachments
        paintMessageContent(painter, option, msg);
    }
    painter->restore();
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    const Message msg = index.data(MessageModel::RawMessageRole).value<Message>();
    if (msg.layoutCache) {
        // Allow a small delta to prevent oscillating fallbacks during resize
        if (qAbs(msg.layoutCache->lastViewWidth - option.rect.width()) <= 2) {
            return {option.rect.width(), msg.layoutCache->totalHeight};
        }
    }

    const int viewWidth = option.rect.width() > 0 ? option.rect.width() : 600;
    // Fallback if not precomputed or width changed significantly
    if (msg.layoutCache) {
        return {viewWidth, msg.layoutCache->totalHeight};
    }
    return {viewWidth, kAvatarSize + 2 * kRowMargin};
}

}  // namespace CodeHex