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
        QPixmap pix(":/icons/app.png");
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

        if (block.type == BlockType::Text) {
            int x = isUser ? viewWidth - bl.width - kAvatarSize - 12 : kAvatarSize + 12;

            // Bubble background
            const QColor bubbleColor = isUser ? QColor(0x2563EB) : QColor(0x374151);
            p->setPen(Qt::NoPen);
            p->setBrush(bubbleColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bl.width, bl.height, kBubbleRadius, kBubbleRadius);

            // Text
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, Qt::white);
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();

            currentY += bl.height + kRowMargin;
        } else if (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua) {
            int x = kAvatarSize + 12;

            // Bubble background (gray for code)
            const QColor codeColor(0x1F2937);
            p->setPen(Qt::NoPen);
            p->setBrush(codeColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bl.width, bl.height, kBubbleRadius, kBubbleRadius);

            // Text
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, QColor(0x9CA3AF));
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();

            currentY += bl.height + kRowMargin;
        } else if (block.type == BlockType::Thinking) {
            const int headerH = 24;
            int x = kAvatarSize + 12;

            // Thinking Bubble (Subtle glassmorphism)
            const QColor thinkingBg(45, 55, 72, 180); // Semi-transparent dark slate
            p->setPen(Qt::NoPen);
            p->setBrush(thinkingBg);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bl.width, bl.height, kBubbleRadius, kBubbleRadius);

            // "Thinking..." Header
            p->setPen(QColor(0xA0AEC0));
            QFont headerFont = opt.font;
            headerFont.setItalic(true);
            headerFont.setPointSizeF(headerFont.pointSizeF() * 0.85);
            p->setFont(headerFont);
            p->drawText(QRect(x + kBubblePadding, currentY + 4, bl.width - 2 * kBubblePadding, headerH), 
                        Qt::AlignVCenter | Qt::AlignLeft, "💭 Agent thinking...");

            // Thought content
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding + headerH);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, QColor(0xCBD5E0));
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();

            p->setFont(opt.font);
            currentY += bl.height + kRowMargin;
        } else if (block.type == BlockType::Output) {
            const int headerH = 24;
            int x = kAvatarSize + 12;

            // Bubble background
            const QColor outputColor(0x111827);
            p->setPen(Qt::NoPen);
            p->setBrush(outputColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bl.width, bl.height, kBubbleRadius, kBubbleRadius);

            // Header
            p->setPen(QColor(0x10B981));
            QFont headerFont = opt.font;
            headerFont.setBold(true);
            headerFont.setPointSizeF(headerFont.pointSizeF() * 0.9);
            p->setFont(headerFont);
            p->drawText(QRect(x + kBubblePadding, currentY + 6, bl.width - 2 * kBubblePadding, headerH), 
                        Qt::AlignVCenter | Qt::AlignLeft, "✅ Tool Result");

            // Text content
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding + headerH);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, QColor(0xD1D5DB));
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();

            p->setFont(opt.font);
            currentY += bl.height + kRowMargin;
        } else if (block.type == BlockType::ToolCall) {
            const int x = kAvatarSize + 12;

            const QColor toolCallColor(0x374151);
            p->setPen(Qt::NoPen);
            p->setBrush(toolCallColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bl.width, bl.height, kBubbleRadius, kBubbleRadius);

            p->setPen(QColor(0x60A5FA));
            p->drawText(QRect(x + kBubblePadding, currentY, bl.width - 2 * kBubblePadding, bl.height),
                        Qt::AlignCenter | Qt::AlignLeft, "⚙  " + block.content);

            currentY += bl.height + kRowMargin;
        }
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
    if (msg.layoutCache && msg.layoutCache->lastViewWidth == option.rect.width()) {
        return {option.rect.width(), msg.layoutCache->totalHeight};
    }

    const int viewWidth = option.rect.width() > 0 ? option.rect.width() : 600;
    // Fallback if not precomputed or width changed (should be handled by setViewWidth)
    return {viewWidth, kAvatarSize + 2 * kRowMargin};
}

}  // namespace CodeHex