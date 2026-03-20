#pragma once
#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>

class QLabel;

namespace CodeHex {

class WorkFolderPanel : public QWidget {
    Q_OBJECT
public:
    explicit WorkFolderPanel(QWidget* parent = nullptr);
    void setFolder(const QString& path);

signals:
    void folderChanged(const QString& newPath);
    void fileSelected(const QString& filePath);

private slots:
    void onBrowse();
    void onRefresh();

private:
    QTreeView* m_treeView;
    QFileSystemModel* m_model;
    QLabel* m_pathLabel;
    QString m_currentPath;
};

} // namespace CodeHex
