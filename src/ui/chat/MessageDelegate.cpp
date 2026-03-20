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
    p->setRenderHint(QPainter::Antialiasing);
    
    if (isUser) {
        // Modern User Avatar (Blue Circle with Initial)
        QRadialGradient grad(avatarX + kAvatarSize/2, avatarY + kAvatarSize/2, kAvatarSize/2);
        grad.setColorAt(0, QColor(0x60A5FA)); // Brighter Blue
        grad.setColorAt(1, QColor(0x2563EB)); // Deep Blue
        p->setBrush(grad);
        p->setPen(Qt::NoPen);
        p->drawEllipse(avatarX, avatarY, kAvatarSize, kAvatarSize);
        
        p->setPen(Qt::white);
        QFont font = p->font();
        font.setBold(true);
        font.setPixelSize(14);
        p->setFont(font);
        p->drawText(QRect(avatarX, avatarY, kAvatarSize, kAvatarSize), Qt::AlignCenter, "U");
    } else {
        // Agent Avatar (App Icon with subtle glow)
        QPixmap pix(":/resources/icons/app.png");
        if (!pix.isNull()) {
             p->setOpacity(0.9);
             p->drawPixmap(avatarX, avatarY, kAvatarSize, kAvatarSize, pix);
             p->setOpacity(1.0);
        } else {
            QRadialGradient grad(avatarX + kAvatarSize/2, avatarY + kAvatarSize/2, kAvatarSize/2);
            grad.setColorAt(0, QColor(0x34D399)); // Emerald
            grad.setColorAt(1, QColor(0x059669));
            p->setBrush(grad);
            p->setPen(Qt::NoPen);
            p->drawEllipse(avatarX, avatarY, kAvatarSize, kAvatarSize);
            p->setPen(Qt::white);
            p->drawText(QRect(avatarX, avatarY, kAvatarSize, kAvatarSize), Qt::AlignCenter, "🤖");
        }
    }

    if (!msg.layoutCache) return;
    const auto& layout = *msg.layoutCache;

    int currentBlockIdx = 0;
    for (const CodeBlock& block : msg.contentBlocks) {
        if (currentBlockIdx >= layout.blocks.size()) break;
        const auto& bl = layout.blocks[currentBlockIdx++];

        int x = isUser ? viewWidth - bl.width - kAvatarSize - 12 : kAvatarSize + 12;
        if (block.type != BlockType::Text) x = kAvatarSize + 12;

        // --- Bubble background ---
        QColor bubbleColor;
        if (isUser) {
            bubbleColor = QColor(0x2563EB);
        } else {
            switch(block.type) {
                case BlockType::Text:     bubbleColor = QColor(0x374151); break;
                case BlockType::Bash:
                case BlockType::Python:
                case BlockType::Lua:      bubbleColor = QColor(0x111827); break; // Darker command-block style
                case BlockType::Output:    bubbleColor = QColor(0x0F172A); break;
                case BlockType::Thinking:  bubbleColor = QColor(88, 80, 236, 30); break; // Faint Indigo
                case BlockType::ToolCall:  bubbleColor = QColor(0x1F2937); break;
                case BlockType::LogStep:   bubbleColor = QColor(0x1F2937); break;
                default:                  bubbleColor = QColor(0x374151); break;
            }
        }

        p->setBrush(bubbleColor);
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(x, currentY, bl.width + kBubblePadding * 2, bl.height, kBubbleRadius, kBubbleRadius);

        int contentOffset = kBubblePadding;

        // --- Header / Collapsed Rendering ---
        if (block.type == BlockType::Thinking || block.type == BlockType::ToolCall || block.type == BlockType::LogStep) {
            QString header;
            QColor headerColor = QColor(0x9CA3AF); // TEXT_DIM
            
            if (block.isCollapsed) {
                header = "▶  ";
                if (block.type == BlockType::Thinking) header += "Thought";
                else {
                    const QString c = block.content.toLower();
                    if (c.contains("list") || c.contains("read") || c.contains("view")) header += "Analyzed";
                    else if (c.contains("search") || c.contains("find") || c.contains("grep")) header += "Searched";
                    else if (c.contains("command") || c.contains("cli")) header += "Ran command";
                    else header += "Tool call";
                }
                p->setPen(headerColor);
                p->drawText(QRect(x + kBubblePadding, currentY, bl.width, bl.height), Qt::AlignVCenter, header);
            } else {
                header = "▼  ";
                if (block.type == BlockType::Thinking) {
                    header += "Reasoning";
                    headerColor = QColor(0xA5B4FC); // Soft indigo
                } else {
                    const QString c = block.content.toLower();
                    if (c.contains("list") || c.contains("read") || c.contains("view")) header += "📄 Analyzed";
                    else if (c.contains("search") || c.contains("find") || c.contains("grep")) header += "🔍 Searched";
                    else if (c.contains("command") || c.contains("cli")) header += "🐚 Ran command";
                    else header += "⚙  Action";
                }
                
                p->setPen(headerColor);
                p->drawText(QRect(x + kBubblePadding, currentY + 6, bl.width, 22), Qt::AlignTop, header);
                
                // Separator line
                p->setPen(QPen(headerColor.darker(150), 1));
                p->drawLine(x + kBubblePadding, currentY + 28, x + bl.width + kBubblePadding, currentY + 28);
                contentOffset = 32;
            }
        }

        // --- Drawing Document Content (if expanded or not a step block) ---
        if (!block.isCollapsed) {
            p->save();
            p->translate(x + kBubblePadding, currentY + contentOffset);
            QAbstractTextDocumentLayout::PaintContext ctx;
            QColor textColor = isUser ? Qt::white : QColor(0xF8FAFC);
            
            if (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua)
                textColor = QColor(0xCBD5E1);
            else if (block.type == BlockType::Output)
                textColor = QColor(0xD1D5DB);
            else if (block.type == BlockType::Thinking)
                textColor = QColor(0x94A3B8);
                
            ctx.palette.setColor(QPalette::Text, textColor);
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();
        }

        currentY += bl.height + 12; // Spacing
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

int MessageDelegate::blockIndexAt(const QPoint& pos, const QRect& rect, const Message& msg) const {
    if (!msg.layoutCache) return -1;
    
    const bool isUser = (msg.role == Message::Role::User);
    int currentY = rect.top() + 10; // kRowMargin

    int idx = 0;
    for (const auto& block : msg.contentBlocks) {
        if (idx >= msg.layoutCache->blocks.size()) break;
        const auto& bl = msg.layoutCache->blocks[idx];

        int x = isUser ? rect.width() - bl.width - 32 - 12 : 32 + 12; // Roughly matching paint logic
        if (block.type != BlockType::Text) x = 32 + 12;

        QRect bubbleRect(x, currentY, bl.width + 12 * 2, bl.height);
        if (bubbleRect.contains(pos)) {
            if (block.type == BlockType::Thinking || block.type == BlockType::ToolCall) {
                return idx;
            }
        }
        currentY += bl.height + 12;
        idx++;
    }
    return -1;
}

}  // namespace CodeHex
