#pragma once
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QTimer>
#include "../cli/ProfileEntry.h"
#include "../data/Attachment.h"
#include "../data/Message.h"
#include "../data/Session.h"
#include "widgets/TerminalPanel.h"
#include "widgets/ThemeManager.h"
#include "help/HelpDialog.h"
#include "chat/ToolApprovalDialog.h"

class LlmDiscoveryService;
class ChatControlBanner;
class ChatView;
class MessageModel;
class InputPanel;
class ConsoleWidget;
class WorkFolderPanel;
class SessionPanel;
class QSlider;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppConfig* config,
                        SessionManager* sessions,
                        ChatController* controller,
                        AudioRecorder* recorder,
                        AudioPlayer* player,
                        const ProfileList& extraProfiles = {},
                        QWidget* parent = nullptr);

private slots:
    void onSendRequested(const QString& text, const QList<Attachment>& attachments);
    void onStopRequested();
    void onSessionSelected(const QString& id);
    void onNewSessionRequested();
    void onTokenReceived(const QString& token);
    void onResponseComplete(const Message& msg);
    void onGenerationStarted();
    void onGenerationStopped();
    void onClearChatRequested();
    void onThemeToggleRequested();
    void onToolApprovalRequested(const QString& toolName, const QJsonObject& input);
    void onProfileChanged(int index);
    void onHelpRequested(const QString& page = "getting-started");
    void onAbout();
    void onTokenBufferTimeout();
    void onCommandRequested(const QString& cmd, const QStringList& args);
    void onLlmSliderChanged(int value);
    void onModelSelected(int index);
    void onModelsReady(const QStringList& models);
    void onDiscoveryError(const QString& error);

private:
    void setupUi();
    void setupMenuBar();
    void loadStyleSheet();
    void switchSession(Session* session);
    void populateProfileCombo();

    AppConfig*      m_config;
    SessionManager* m_sessions;
    ChatController* m_controller;
    AudioRecorder*  m_recorder;
    AudioPlayer*    m_player;
    ProfileList     m_extraProfiles;

    // UI widgets
    QSplitter*          m_splitter = nullptr;
    QSplitter*          m_sidebarSplitter = nullptr;
    SessionPanel*       m_sessionPanel = nullptr;
    WorkFolderPanel*    m_workFolderPanel = nullptr;
    ChatView*           m_chatView = nullptr;
    MessageModel*       m_messageModel;
    InputPanel*         m_inputPanel;
    TerminalPanel*      m_terminalPanel;
    ChatControlBanner*  m_chatBanner;
    LlmDiscoveryService* m_discoveryService;
    QSlider*            m_llmSlider;     // Privacy vs Performance
    QComboBox*          m_modelCombo;    // Dynamic model discovery
    QComboBox*          m_profileCombo;  // Legacy (to be phased out)
    QComboBox*          m_roleCombo;
    QCheckBox*          m_autoApproveCheck;
    QPushButton*        m_scrollToBottomBtn;
    QPushButton*        m_autoScrollBtn; // Magnet button
    QPushButton*        m_stopBtn;       // Stop agent button
    QPushButton*        m_themeBtn;      // Dark/Light toggle
    QLabel*             m_statusLabel;   // "Agent is thinking..."
    QLabel*             m_tokenLabel   = nullptr;

    // Streaming state
    QString m_streamingText;
    bool    m_hasStreamingMsg  = false;
    bool    m_isStreaming       = false;
    bool    m_cursorVisible     = false;
    QTimer* m_cursorTimer       = nullptr;
    
    // Token batching
    QString m_tokenBuffer;
    QTimer* m_tokenTimer        = nullptr;

    HelpDialog* m_helpDialog = nullptr;

    void updateTokenLabel();
    void onCursorBlink();
};

}  // namespace CodeHex
