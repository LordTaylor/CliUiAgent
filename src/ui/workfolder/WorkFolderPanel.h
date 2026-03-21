#pragma once
#include <QWidget>
#include <QSet>
#include <QString>
#include <QFileSystemModel>
#include <QPoint>

class QLabel;
class QTreeView;
class QPushButton;
class QFileSystemModel;
class QSortFilterProxyModel;

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
    void onToggleHotness();

private:
    void setupUi();

    QString m_currentPath;
    QLabel* m_pathLabel = nullptr;
    QTreeView* m_treeView = nullptr;
    QFileSystemModel* m_model = nullptr;
    class SmartFileSortProxyModel* m_proxy = nullptr;
    class FileHotnessProvider* m_hotness = nullptr;
    QPushButton* m_hotnessBtn = nullptr;
    
    QSet<QString> m_forcedContextFiles;
};

} // namespace CodeHex
