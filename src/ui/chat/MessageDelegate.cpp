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
        QColor borderColor = Qt::transparent;
        bool isHollow = false;

        if (isUser) {
            bubbleColor = QColor(0x2D241C); // Deep Brown
            borderColor = QColor(0xD97706); // Amber
        } else {
            switch(block.type) {
                case BlockType::Text:     
                    bubbleColor = QColor(20, 15, 10, 200); // Obsidian semi-transparent
                    borderColor = QColor(217, 119, 6, 50); 
                    break;
                case BlockType::Bash:
                case BlockType::Python:
                case BlockType::Lua:      
                    bubbleColor = QColor(15, 11, 8); 
                    borderColor = QColor(69, 54, 43);
                    break;
                case BlockType::Output:    
                    bubbleColor = QColor(26, 18, 10); 
                    borderColor = QColor(34, 197, 94, 100); // Greenish touch for "DONE"
                    break;
                case BlockType::Thinking:  
                    bubbleColor = Qt::transparent; 
                    borderColor = QColor(217, 119, 6, 120);
                    isHollow = true;
                    break;
                case BlockType::ToolCall:  
                    bubbleColor = Qt::transparent;
                    borderColor = QColor(217, 119, 6);
                    isHollow = true;
                    break;
                default:                  
                    bubbleColor = QColor(0x1F2937); 
                    break;
            }
        }

        p->setBrush(isHollow ? Qt::NoBrush : bubbleColor);
        p->setPen(borderColor == Qt::transparent ? Qt::NoPen : QPen(borderColor, 1.5));
        p->drawRoundedRect(x, currentY, bl.width + kBubblePadding * 2, bl.height, kBubbleRadius, kBubbleRadius);

        int contentOffset = kBubblePadding;

        // --- Header / Collapsed Rendering ---
        if (block.type == BlockType::Thinking || block.type == BlockType::ToolCall || block.type == BlockType::LogStep) {
            QString header;
            QColor headerColor = QColor(0xD97706); // Amber
            
            if (block.isCollapsed) {
                header = "▶  ";
                if (block.type == BlockType::Thinking) header += "Thinking...";
                else {
                    const QString c = block.content.toLower();
                    header += QString("Call: ") + (c.contains("bash") ? "Bash" : c.contains("python") ? "Python" : "Tool");
                }
                p->setPen(headerColor);
                p->drawText(QRect(x + kBubblePadding, currentY, bl.width, bl.height), Qt::AlignVCenter, header);
            } else {
                header = "▼  ";
                if (block.type == BlockType::Thinking) {
                    header += "Reasoning";
                } else {
                    const QString c = block.content.toLower();
                    header += QString("Executing: ") + (c.contains("bash") ? "Bash" : "Agent Action");
                }
                
                p->setPen(headerColor);
                p->drawText(QRect(x + kBubblePadding, currentY + 8, bl.width, 22), Qt::AlignTop, header);
                
                // Separator line
                p->setPen(QPen(headerColor.darker(150), 1));
                p->drawLine(x + kBubblePadding, currentY + 30, x + bl.width + kBubblePadding, currentY + 30);
                contentOffset = 36;
            }
        } else if (block.type == BlockType::Output) {
            // "DONE" style for output blocks
            p->setPen(QColor(0x22C55E)); // Green
            p->drawText(QRect(x + kBubblePadding, currentY + 6, bl.width, 22), Qt::AlignTop, "✓ DONE");
            contentOffset = 28;
        }

        // --- Drawing Document Content (if expanded or not a step block) ---
        if (!block.isCollapsed) {
            p->save();
            p->translate(x + kBubblePadding, currentY + contentOffset);
            QAbstractTextDocumentLayout::PaintContext ctx;
            QColor textColor = isUser ? QColor(0xE2E2E2) : QColor(0xD9D9D9);
            
            if (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua)
                textColor = QColor(0xFBBF24); // Amber/Gold for code
            else if (block.type == BlockType::Output)
                textColor = QColor(0xA1A1AA);
            else if (block.type == BlockType::Thinking)
                textColor = QColor(0x71717A);
                
            ctx.palette.setColor(QPalette::Text, textColor);
            bl.doc->documentLayout()->draw(p, ctx);
            p->restore();

            // Draw Copy button for code blocks
            if (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua) {
                p->save();
                int copyBtnX = x + bl.width + kBubblePadding * 2 - 28;
                int copyBtnY = currentY + 8;
                QRect copyRect(copyBtnX, copyBtnY, 20, 20);
                
                p->setPen(QColor(156, 163, 175)); // Gray-400
                QFont iconFont = p->font();
                iconFont.setPointSize(12);
                p->setFont(iconFont);
                p->drawText(copyRect, Qt::AlignCenter, "📋"); // Clipboard icon
                p->restore();
            }
        }

        currentY += bl.height + 12; // Spacing
    }

    // Confidence badge — shown for assistant messages when model emitted a score
    if (!isUser && msg.confidenceScore >= 0) {
        const QString badgeText = QString("◆ %1/10").arg(msg.confidenceScore);
        QFont badgeFont = opt.font;
        badgeFont.setPointSizeF(badgeFont.pointSizeF() * 0.78);
        p->setFont(badgeFont);
        const QFontMetrics fm(badgeFont);
        const int textW   = fm.horizontalAdvance(badgeText) + 16;
        const int badgeX  = kAvatarSize + 12;
        const int badgeY  = currentY;

        // Color: green >=7, amber 4-6, red <=3
        QColor bgColor = (msg.confidenceScore >= 7) ? QColor(0x065F46)
                       : (msg.confidenceScore >= 4) ? QColor(0x78350F)
                       :                              QColor(0x7F1D1D);
        QColor textColor = (msg.confidenceScore >= 7) ? QColor(0x34D399)
                         : (msg.confidenceScore >= 4) ? QColor(0xFBBF24)
                         :                              QColor(0xFCA5A5);

        p->setPen(Qt::NoPen);
        p->setBrush(bgColor);
        p->drawRoundedRect(badgeX, badgeY, textW, kBadgeHeight, 6, 6);
        p->setPen(textColor);
        p->drawText(QRect(badgeX + 8, badgeY, textW - 8, kBadgeHeight),
                    Qt::AlignVCenter | Qt::AlignLeft, badgeText);
        p->setFont(opt.font);
        currentY += kBadgeHeight + 4;
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

    // Draw Eye button for toggling thinking
    if (msg.role == Message::Role::Assistant) {
        bool hasThinking = false;
        for (const auto& b : msg.contentBlocks) {
            if (b.type == BlockType::Thinking) { hasThinking = true; break; }
        }
        if (hasThinking) {
            const int viewWidth = option.rect.width();
            int btnX = viewWidth - 32 - 12; // Increased hit area
            int btnY = option.rect.top() + 8;
            QRect btnRect(btnX, btnY, 32, 32); // 32x32 hit area instead of 24x24
            
            painter->setPen(QColor(156, 163, 175));
            QFont iconFont = painter->font();
            iconFont.setPointSize(16); // Slightly larger icon
            painter->setFont(iconFont);
            painter->drawText(btnRect, Qt::AlignCenter, msg.showThinking ? "👁" : "👁‍🗨");
        }
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

int MessageDelegate::copyBlockIndexAt(const QPoint& pos, const QRect& rect, const Message& msg) const {
    if (!msg.layoutCache) return -1;
    
    const bool isUser = (msg.role == Message::Role::User);
    int currentY = rect.top() + 10;

    int idx = 0;
    for (const auto& block : msg.contentBlocks) {
        if (idx >= msg.layoutCache->blocks.size()) break;
        const auto& bl = msg.layoutCache->blocks[idx];

        int x = isUser ? rect.width() - bl.width - 32 - 12 : 32 + 12;
        if (block.type != BlockType::Text) x = 32 + 12;

        if (!block.isCollapsed && (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua)) {
            int copyBtnX = x + bl.width + kBubblePadding * 2 - 28;
            int copyBtnY = currentY + 8;
            QRect copyRect(copyBtnX, copyBtnY, 20, 20);
            if (copyRect.contains(pos)) {
                return idx;
            }
        }
        currentY += bl.height + 12;
        idx++;
    }
    return -1;
}

bool MessageDelegate::isEyeButtonClicked(const QPoint& pos, const QRect& rect, const Message& msg) const {
    if (msg.role != Message::Role::Assistant) return false;
    if (!msg.layoutCache) return false;

    bool hasThinking = false;
    for (const auto& b : msg.contentBlocks) {
        if (b.type == BlockType::Thinking) { hasThinking = true; break; }
    }
    if (!hasThinking) return false;

    const int viewWidth = rect.width();
    int btnX = viewWidth - 24 - 12;
    int btnY = rect.top() + 10;
    QRect btnRect(btnX, btnY, 24, 24);
    
    return btnRect.contains(pos);
}

}  // namespace CodeHex
