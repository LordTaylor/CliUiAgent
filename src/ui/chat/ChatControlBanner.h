#ifndef CHATCONTROLBANNER_H
#define CHATCONTROLBANNER_H

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QString>

namespace CodeHex {

class ChatControlBanner : public QWidget {
    Q_OBJECT
public:
    explicit ChatControlBanner(QWidget* parent = nullptr);

    void setAutoApprove(bool enabled);
    bool autoApprove() const;
    void setModelName(const QString& name);
    void setThinking(bool thinking);
    void setStatusText(const QString& text);

signals:
    void autoApproveChanged(bool enabled);
    void clearChatRequested();

private slots:
    void onPulse();

private:
    QCheckBox* m_autoApproveCheck;
    QLabel* m_modelLabel;
    QLabel* m_statusLabel;
    QPushButton* m_clearBtn;
    QTimer* m_pulseTimer;
    int m_pulseAlpha = 255;
    bool m_pulseDir = true;
};

} // namespace CodeHex

#endif // CHATCONTROLBANNER_H
