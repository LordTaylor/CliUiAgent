#pragma once
#include <QDialog>
#include <QTextBrowser>
#include <QListWidget>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>

namespace CodeHex {

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(const QString& startPage = "index", QWidget* parent = nullptr);

    void openPage(const QString& page);

private slots:
    void onPageSelected(QListWidgetItem* item);
    void onBack();
    void onForward();

private:
    void setupUi();
    void loadPage(const QString& resourcePath);
    void updateNavButtons();

    QSplitter*    m_splitter;
    QListWidget*  m_toc;
    QTextBrowser* m_browser;
    QPushButton*  m_backBtn;
    QPushButton*  m_fwdBtn;
    QLabel*       m_titleLabel;

    QStringList   m_history;
    int           m_historyPos = -1;
};

}  // namespace CodeHex
