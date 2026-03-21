#pragma once
#include <QList>
#include <QToolButton>
#include "../../data/Attachment.h"

namespace CodeHex {

class AttachmentButton : public QToolButton {
    Q_OBJECT
public:
    explicit AttachmentButton(QWidget* parent = nullptr);

    QList<Attachment> pendingAttachments() const;
    void clearAttachments();
    void addAttachments(const QList<Attachment>& attachments);
    void removeAttachment(int index); // New

signals:
    void attachmentsChanged(const QList<Attachment>& attachments);

private slots:
    void onClicked();

private:
    QList<Attachment> m_attachments;
};

}  // namespace CodeHex
