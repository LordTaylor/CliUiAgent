#include "WorkFolderPanel.h"
#include "SmartFileSortProxyModel.h"
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QString>
#include "../../core/FileHotnessProvider.h"
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>

namespace CodeHex {

WorkFolderPanel::WorkFolderPanel(QWidget* parent) : QWidget(parent) {
    m_hotness = new FileHotnessProvider(this);
    setupUi();
}

void WorkFolderPanel::setupUi() {
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

    m_hotnessBtn = new QPushButton("🔥", header);
    m_hotnessBtn->setCheckable(true);
    m_hotnessBtn->setFixedSize(24, 24);
    m_hotnessBtn->setToolTip("Sort by Hotness (Git History)");
    m_hotnessBtn->setStyleSheet("background: transparent; border: none; font-size: 14px;");
    connect(m_hotnessBtn, &QPushButton::toggled, this, &WorkFolderPanel::onToggleHotness);
    hLayout->addWidget(m_hotnessBtn);

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

    m_proxy = new SmartFileSortProxyModel(m_hotness, this);
    m_proxy->setSourceModel(m_model);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_proxy);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(15);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Hide size, type, date columns
    for (int i = 1; i < 4; ++i) m_treeView->hideColumn(i);

    connect(m_treeView, &QTreeView::clicked, this, [this](const QModelIndex& index) {
        QModelIndex sourceIndex = m_proxy->mapToSource(index);
        if (!m_model->isDir(sourceIndex)) {
            emit fileSelected(m_model->filePath(sourceIndex));
        }
    });

    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &WorkFolderPanel::onCustomContextMenuRequested);

    layout->addWidget(m_treeView, 1);
}

void WorkFolderPanel::onToggleHotness() {
    bool enabled = m_hotnessBtn->isChecked();
    m_proxy->setSortByHotness(enabled);
    if (enabled) {
        m_treeView->sortByColumn(0, Qt::DescendingOrder); // Hot files first
    } else {
        m_treeView->sortByColumn(0, Qt::AscendingOrder);  // Alphabetical
    }
}

void WorkFolderPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    QModelIndex sourceIndex = m_proxy->mapToSource(index);
    if (!sourceIndex.isValid() || m_model->isDir(sourceIndex)) return;

    QString filePath = m_model->filePath(sourceIndex);
    bool isIncluded = m_forcedContextFiles.contains(filePath);

    QMenu menu(this);
    QAction* toggleAct = menu.addAction(isIncluded ? "Remove from Context" : "Add to Context");
    
    if (menu.exec(m_treeView->viewport()->mapToGlobal(pos)) == toggleAct) {
        if (isIncluded) m_forcedContextFiles.remove(filePath);
        else m_forcedContextFiles.insert(filePath);
        
        emit contextFilesChanged(m_forcedContextFiles);
        onRefresh();
    }
}

void WorkFolderPanel::onRefresh() {
    if (!m_currentPath.isEmpty()) {
        m_hotness->scan(m_currentPath);
        m_model->setRootPath(""); // Reset
        m_model->setRootPath(m_currentPath); // Re-trigger scan
    }
}

void WorkFolderPanel::setFolder(const QString& path) {
    m_currentPath = path;
    m_hotness->scan(path);
    m_model->setRootPath(path);
    m_treeView->setRootIndex(m_proxy->mapFromSource(m_model->index(path)));
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
