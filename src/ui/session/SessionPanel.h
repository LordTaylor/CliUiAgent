#pragma once
#include <QWidget>
#include <QObject>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace CodeHex {

class SessionManager;
class Session;

class SessionPanel : public QWidget {
    Q_OBJECT
public:
    explicit SessionPanel(SessionManager* manager, QWidget* parent = nullptr);

    void refresh();
    void selectSession(const QString& sessionId);

signals:
    void sessionSelected(const QString& sessionId);
    void newSessionRequested();

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onItemChanged(QListWidgetItem* item);
    void onNewClicked();
    void onDeleteClicked();

private:
    SessionManager* m_manager;
    QListWidget* m_list;
    QPushButton* m_newBtn;
    QPushButton* m_deleteBtn;
};

}  // namespace CodeHex
