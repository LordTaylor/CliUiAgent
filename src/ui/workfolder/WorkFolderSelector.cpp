#include "WorkFolderSelector.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace CodeHex {

WorkFolderSelector::WorkFolderSelector(QWidget* parent) : QWidget(parent) {
    setObjectName("folderSelector");
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    m_btn = new QToolButton(this);
    m_btn->setObjectName("folderBtn");
    m_btn->setToolTip("Set working folder");
    m_btn->setText("📁");
    m_btn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_btn);

    m_label = new QLabel("(no folder)", this);
    m_label->setObjectName("folderLabel");
    m_label->setMinimumWidth(100);
    layout->addWidget(m_label, 1);

    connect(m_btn, &QToolButton::clicked, this, &WorkFolderSelector::onBrowse);
}

QString WorkFolderSelector::folder() const { return m_folder; }

void WorkFolderSelector::setFolder(const QString& path) {
    if (m_folder == path) return;
    m_folder = path;
    m_label->setText(path.isEmpty() ? "(no folder)" : path);
    emit folderChanged(m_folder);
}

void WorkFolderSelector::onBrowse() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Select Working Folder",
        m_folder.isEmpty() ? QDir::homePath() : m_folder,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) setFolder(dir);
}

}  // namespace CodeHex
