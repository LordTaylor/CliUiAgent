#pragma once
#include <QDialog>
#include <QStringList>

class QListWidget;

namespace CodeHex {

class PluginsDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginsDialog(QWidget* parent = nullptr);
    void setScratchpadPath(const QString& path);

private slots:
    void onRefresh();
    void onRunScript();

private:
    void setupUi();
    void loadScripts();

    QListWidget* m_list;
    QString m_scratchpadPath;
};

} // namespace CodeHex
