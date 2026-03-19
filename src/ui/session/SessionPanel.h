#pragma once
#include <QWidget>

class QListWidget;
class QPushButton;

namespace CodeHex {

class SessionManager;
class Session;

class SessionPanel : public QWidget {
    Q_OBJECT
public:
    explicit SessionPanel(SessionManager* manager, QWidget* parent = nullptr);

    void refresh();

signals:
    void sessionSelected(const QString& sessionId);
    void newSessionRequested();

private slots:
    void onItemDoubleClicked();
    void onNewClicked();
    void onDeleteClicked();

private:
    SessionManager* m_manager;
    QListWidget* m_list;
    QPushButton* m_newBtn;
    QPushButton* m_deleteBtn;
};

}  // namespace CodeHex
