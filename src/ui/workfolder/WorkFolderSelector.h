#pragma once
#include <QWidget>

class QLabel;
class QToolButton;

namespace CodeHex {

class WorkFolderSelector : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
public:
    explicit WorkFolderSelector(QWidget* parent = nullptr);

    QString folder() const;
    void setFolder(const QString& path);

signals:
    void folderChanged(const QString& path);

private slots:
    void onBrowse();

private:
    QToolButton* m_btn;
    QLabel* m_label;
    QString m_folder;
};

}  // namespace CodeHex
