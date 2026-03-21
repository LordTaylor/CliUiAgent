#include "WorkFolderPanel.h"
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QHeaderView>
#include <QDir>
#include <QMenu>
#include <QAction>

namespace CodeHex {

WorkFolderPanel::WorkFolderPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- Header ---
    auto* header = new QWidget(this);
    header->setObjectName("workFolderHeader");
    auto* hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(8, 6, 8, 6);

    auto* iconLabel = new QLabel("📁", header);
    hLayout->addWidget(iconLabel);

    m_pathLabel = new QLabel("No folder selected", header);
    m_pathLabel->setObjectName("workFolderPathLabel");
    hLayout->addWidget(m_pathLabel, 1);

    auto* browseBtn = new QPushButton("Change", header);
    browseBtn->setObjectName("workFolderBrowseBtn");
    browseBtn->setFixedWidth(60);
    connect(browseBtn, &QPushButton::clicked, this, &WorkFolderPanel::onBrowse);
    hLayout->addWidget(browseBtn);

    auto* refreshBtn = new QPushButton("🔄", header);
    refreshBtn->setFixedSize(24, 24);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet("background: transparent; border: none; font-size: 14px;");
    connect(refreshBtn, &QPushButton::clicked, this, &WorkFolderPanel::onRefresh);
    hLayout->addWidget(refreshBtn);

    layout->addWidget(header);

    // --- Tree View ---
    m_model = new QFileSystemModel(this);
    m_model->setReadOnly(true);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(15);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Hide size, type, date columns
    for (int i = 1; i < 4; ++i) m_treeView->hideColumn(i);

    connect(m_treeView, &QTreeView::clicked, this, [this](const QModelIndex& index) {
        if (!m_model->isDir(index)) {
            emit fileSelected(m_model->filePath(index));
        }
    });

    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &WorkFolderPanel::onCustomContextMenuRequested);

    layout->addWidget(m_treeView, 1);
}

void WorkFolderPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid() || m_model->isDir(index)) return;

    QString filePath = m_model->filePath(index);
    bool isIncluded = m_forcedContextFiles.contains(filePath);

    QMenu menu(this);
    QAction* toggleAct = menu.addAction(isIncluded ? "Remove from Context" : "Add to Context");
    
    if (menu.exec(m_treeView->viewport()->mapToGlobal(pos)) == toggleAct) {
        if (isIncluded) m_forcedContextFiles.remove(filePath);
        else m_forcedContextFiles.insert(filePath);
        
        emit contextFilesChanged(m_forcedContextFiles);
        onRefresh(); // Refresh to potentially show an indicator (though we haven't added one yet)
    }
}

void WorkFolderPanel::onRefresh() {
    if (!m_currentPath.isEmpty()) {
        m_model->setRootPath(""); // Reset
        m_model->setRootPath(m_currentPath); // Re-trigger scan
    }
}

void WorkFolderPanel::setFolder(const QString& path) {
    m_currentPath = path;
    m_model->setRootPath(path);
    m_treeView->setRootIndex(m_model->index(path));
    m_pathLabel->setText(QDir(path).dirName());
    m_pathLabel->setToolTip(path);
}

void WorkFolderPanel::onBrowse() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Work Folder", m_currentPath);
    if (!dir.isEmpty()) {
        setFolder(dir);
        emit folderChanged(dir);
    }
}

} // namespace CodeHex
