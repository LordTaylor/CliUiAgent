#include "AttachmentButton.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeDatabase>

namespace CodeHex {

AttachmentButton::AttachmentButton(QWidget* parent) : QToolButton(parent) {
    setText("📎");
    setToolTip("Attach files or images");
    setCursor(Qt::PointingHandCursor);
    setObjectName("attachBtn");
    connect(this, &QToolButton::clicked, this, &AttachmentButton::onClicked);
}

QList<Attachment> AttachmentButton::pendingAttachments() const { return m_attachments; }

void AttachmentButton::clearAttachments() {
    m_attachments.clear();
    emit attachmentsChanged(m_attachments);
}

void AttachmentButton::onClicked() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Select Attachments", QDir::homePath(),
        "Images & Files (*.png *.jpg *.jpeg *.gif *.webp *.pdf *.txt *.md *.cpp *.h *.py)");

    QMimeDatabase mimeDb;
    for (const QString& path : files) {
        Attachment a;
        a.filePath = path;
        a.sizeBytes = QFileInfo(path).size();
        a.mimeType = mimeDb.mimeTypeForFile(path).name();
        a.type = Attachment::typeFromMime(a.mimeType);
        m_attachments.append(a);
    }
    if (!files.isEmpty()) emit attachmentsChanged(m_attachments);
}

}  // namespace CodeHex
