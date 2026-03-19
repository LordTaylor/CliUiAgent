#include "MessageDelegate.h"
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPixmap>
#include <QTextDocument>
#include <memory>
#include <QFileInfo> // Added
#include "MessageModel.h"
#include "../../data/CodeBlock.h"
#include "../../data/Message.h"

namespace CodeHex {

MessageDelegate::MessageDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

// ── makeTextDoc ───────────────────────────────────────────────────────────────
// Creates a QTextDocument with the given text. When isMarkdown=true the text
// is parsed as CommonMark (Qt6 setMarkdown); otherwise plain text is used.
// The caller takes ownership of the returned pointer.
QTextDocument* MessageDelegate::makeTextDoc(const QString& text, int maxWidth,
                                             bool isMarkdown) const {
    auto* doc = new QTextDocument();
    doc->setPageSize({qreal(maxWidth), -1.0});
    if (isMarkdown)
        doc->setMarkdown(text);
    else
        doc->setPlainText(text);
    return doc;
}

// ── paintMessageContent ───────────────────────────────────────────────────────
// Generic method to paint message content blocks.
void MessageDelegate::paintMessageContent(QPainter* p, const QStyleOptionViewItem& opt,
                                          const Message& msg) const {
    const bool isUser = (msg.role == Message::Role::User);
    const QRect& r = opt.rect;
    const int viewWidth = r.width();
    const int maxW = qMin(kMaxBubbleWidth, viewWidth - 2 * kAvatarSize - 20);

    int currentY = r.top() + kRowMargin;

    for (const CodeBlock& block : msg.contentBlocks) {
        if (block.type == BlockType::Text) {
            std::unique_ptr<QTextDocument> docPtr(makeTextDoc(block.content, maxW - 2 * kBubblePadding, !isUser));
            QTextDocument& doc = *docPtr;
            doc.setDefaultFont(opt.font);
            doc.setTextWidth(maxW - 2 * kBubblePadding);
            const int docH = static_cast<int>(doc.size().height());
            const int bubbleH = docH + 2 * kBubblePadding;
            const int bubbleW = qMin(maxW, static_cast<int>(doc.idealWidth()) + 2 * kBubblePadding);

            int x;
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
            p->drawRoundedRect(x, currentY, bubbleW, bubbleH, kBubbleRadius, kBubbleRadius);

            // Text
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding);
            p->setPen(Qt::white);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, Qt::white);
            doc.documentLayout()->draw(p, ctx);
            p->restore();

            currentY += bubbleH + kRowMargin; // Move Y for next block
        } else if (block.type == BlockType::Bash || block.type == BlockType::Python || block.type == BlockType::Lua) {
            // Render code block
            std::unique_ptr<QTextDocument> docPtr(makeTextDoc(block.content, maxW - 2 * kBubblePadding, true)); // Code is always markdown
            QTextDocument& doc = *docPtr;
            doc.setDefaultFont(opt.font);
            doc.setTextWidth(maxW - 2 * kBubblePadding);
            const int docH = static_cast<int>(doc.size().height());
            const int bubbleH = docH + 2 * kBubblePadding;
            const int bubbleW = qMin(maxW, static_cast<int>(doc.idealWidth()) + 2 * kBubblePadding);

            int x;
            x = kAvatarSize + 12; // Code blocks always align left (assistant output)

            // Bubble background (gray for code)
            const QColor codeColor(0x1F2937);
            p->setPen(Qt::NoPen);
            p->setBrush(codeColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bubbleW, bubbleH, kBubbleRadius, kBubbleRadius);

            // Text
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding);
            p->setPen(QColor(0x9CA3AF)); // Lighter text for code
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, QColor(0x9CA3AF));
            doc.documentLayout()->draw(p, ctx);
            p->restore();

            currentY += bubbleH + kRowMargin; // Move Y for next block
            currentY += bubbleH + kRowMargin; // Move Y for next block
        } else if (block.type == BlockType::Output) {
            // Render CLI output block (Tool Result)
            std::unique_ptr<QTextDocument> docPtr(makeTextDoc(block.content, maxW - 2 * kBubblePadding, false)); 
            QTextDocument& doc = *docPtr;
            doc.setDefaultFont(opt.font);
            doc.setTextWidth(maxW - 2 * kBubblePadding);

            const int headerH = 24;
            const int docH = static_cast<int>(doc.size().height());
            const int bubbleH = docH + 2 * kBubblePadding + headerH;
            const int bubbleW = qMin(maxW, qMax(static_cast<int>(doc.idealWidth()) + 2 * kBubblePadding, 150));

            int x = kAvatarSize + 12;

            // Bubble background (even darker gray for output)
            const QColor outputColor(0x111827);
            p->setPen(Qt::NoPen);
            p->setBrush(outputColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bubbleW, bubbleH, kBubbleRadius, kBubbleRadius);

            // Header: "✅ Tool Result"
            p->setPen(QColor(0x10B981)); // Greenish for success/result
            QFont headerFont = opt.font;
            headerFont.setBold(true);
            headerFont.setPointSizeF(headerFont.pointSizeF() * 0.9);
            p->setFont(headerFont);
            p->drawText(QRect(x + kBubblePadding, currentY + 6, bubbleW - 2 * kBubblePadding, headerH), 
                        Qt::AlignVCenter | Qt::AlignLeft, "✅ Tool Result");

            // Text content
            p->save();
            p->translate(x + kBubblePadding, currentY + kBubblePadding + headerH);
            p->setPen(QColor(0xD1D5DB));
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, QColor(0xD1D5DB));
            doc.documentLayout()->draw(p, ctx);
            p->restore();

            p->setFont(opt.font); // Restore font
            currentY += bubbleH + kRowMargin;
        } else if (block.type == BlockType::ToolCall) {
            // Render Tool Call block
            const int bubbleH = 40;
            const int x = kAvatarSize + 12;
            const int bubbleW = qMin(maxW, 300);

            // Bubble background (subtle blue/gray)
            const QColor toolCallColor(0x374151);
            p->setPen(Qt::NoPen);
            p->setBrush(toolCallColor);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawRoundedRect(x, currentY, bubbleW, bubbleH, kBubbleRadius, kBubbleRadius);

            // Icon and Text: "⚙ <log content>"
            p->setPen(QColor(0x60A5FA)); // Light blue for action/tool
            p->drawText(QRect(x + kBubblePadding, currentY, bubbleW - 2 * kBubblePadding, bubbleH),
                        Qt::AlignCenter | Qt::AlignLeft, "⚙  " + block.content);

            currentY += bubbleH + kRowMargin;
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
    const int viewWidth = option.rect.width() > 0 ? option.rect.width() : 600;
    const int maxW = qMin(kMaxBubbleWidth, viewWidth - 2 * kAvatarSize - 20);

    int totalHeight = 0;
    for (const CodeBlock& block : msg.contentBlocks) {
        // Code blocks should be rendered as markdown for syntax highlighting
        bool isMarkdown = (block.type != BlockType::Text);
        
        if (block.type == BlockType::Output) {
            std::unique_ptr<QTextDocument> doc(makeTextDoc(block.content, maxW - 2 * kBubblePadding, false));
            doc->setDefaultFont(option.font);
            totalHeight += static_cast<int>(doc->size().height()) + 2 * kBubblePadding + 24 + kRowMargin; // 24 for header
        } else if (block.type == BlockType::ToolCall) {
            totalHeight += 40 + kRowMargin;
        } else {
            std::unique_ptr<QTextDocument> doc(makeTextDoc(block.content, maxW - 2 * kBubblePadding, isMarkdown));
            doc->setDefaultFont(option.font);
            totalHeight += static_cast<int>(doc->size().height()) + 2 * kBubblePadding + kRowMargin;
        }
    }

    // Extra height for attachment badge
    if (!msg.attachments.isEmpty())
        totalHeight += kBadgeHeight + kBadgeMargin;

    return {viewWidth, qMax(totalHeight, kAvatarSize + 2 * kRowMargin)};
}

}  // namespace CodeHex