#pragma once
#include <QWidget>
#include <QSet>
#include <QString>
#include <QFileSystemModel>
#include <QPoint>

class QLabel;
class QTreeView;

namespace CodeHex {

class WorkFolderPanel : public QWidget {
    Q_OBJECT
public:
    explicit WorkFolderPanel(QWidget* parent = nullptr);
    void setFolder(const QString& path);
    QSet<QString> forcedContextFiles() const { return m_forcedContextFiles; }

signals:
    void folderChanged(const QString& newPath);
    void fileSelected(const QString& filePath);
    void contextFilesChanged(const QSet<QString>& files);

private slots:
    void onBrowse();
    void onRefresh();
    void onCustomContextMenuRequested(const QPoint& pos);

private:
    QTreeView* m_treeView;
    QFileSystemModel* m_model;
    QLabel* m_pathLabel;
    QString m_currentPath;
    QSet<QString> m_forcedContextFiles;
};

} // namespace CodeHex
