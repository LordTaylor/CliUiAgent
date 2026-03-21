#pragma once
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QProgressBar>
#include <QComboBox>
#include <QLabel>
#include <QShortcut>
#include <QAction>
#include <QDialog>
#include <QPainter>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QRegularExpression>
#include <QMessageBox>
#include <QTimer>
#include <QScrollBar>
#include <QApplication>
#include <QWidget>
#include "../cli/ProfileEntry.h"
#include "../data/Attachment.h"
#include "../data/Message.h"
#include "../data/Session.h"
#include "widgets/TerminalPanel.h"
#include "../core/ThemeManager.h"
#include "help/HelpDialog.h"
#include "chat/ToolApprovalDialog.h"
#include "../core/ContextManager.h"

namespace CodeHex {
    class AppConfig;
    class SessionManager;
    class ChatController;
    class AudioRecorder;
    class AudioPlayer;
    class Session;
    class SessionPanel;
    class WorkFolderPanel;
    class ChatView;
    class MessageModel;
    class InputPanel;
    class TerminalPanel;
    class ChatControlBanner;
    class LlmDiscoveryService;
    class HelpDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(CodeHex::AppConfig* config,
                        CodeHex::SessionManager* sessions,
                        CodeHex::ChatController* controller,
                        CodeHex::AudioRecorder* recorder,
                        CodeHex::AudioPlayer* player,
                        const CodeHex::ProfileList& extraProfiles = {},
                        QWidget* parent = nullptr);

private slots:
    void onSendRequested(const QString& text, const QList<CodeHex::Attachment>& attachments);
    void onStopRequested();
    void onSessionSelected(const QString& id);
    void onNewSessionRequested();
    void onTokenReceived(const QString& token);
    void onResponseComplete(const CodeHex::Message& msg);
    void onGenerationStarted();
    void onGenerationStopped();
    void onClearChatRequested();
    void onThemeToggleRequested();
    void onContextStatsUpdated(const CodeHex::ContextManager::ContextStats& stats);
    void onTokenStatsUpdated(int input, int output);
    void onToolApprovalRequested(const CodeHex::ToolCall& call);
    void onProviderChanged(int index);
    void onHelpRequested(const QString& page = "getting-started");
    void onAbout();
    void onDebugLogRequested();
    void onTokenBufferTimeout();
    void onCommandRequested(const QString& cmd, const QStringList& args);
    // Discovery logic moved to ProviderSettingsDialog

private:
    void setupUi();
    void setupMenuBar();
    void loadStyleSheet();
    void switchSession(CodeHex::Session* session);
    void updateProviderList();

    CodeHex::AppConfig*      m_config;
    CodeHex::SessionManager* m_sessions;
    CodeHex::ChatController* m_controller;
    CodeHex::AudioRecorder*  m_recorder;
    CodeHex::AudioPlayer*    m_player;
    CodeHex::ProfileList     m_extraProfiles;

    // UI widgets
    QSplitter*          m_splitter = nullptr;
    QSplitter*          m_sidebarSplitter = nullptr;
    CodeHex::SessionPanel*       m_sessionPanel = nullptr;
    CodeHex::WorkFolderPanel*    m_workFolderPanel = nullptr;
    CodeHex::ChatView*           m_chatView = nullptr;
    CodeHex::MessageModel*       m_messageModel;
    CodeHex::InputPanel*         m_inputPanel;
    CodeHex::TerminalPanel*      m_terminalPanel;
    CodeHex::ChatControlBanner*  m_chatBanner;
    QLabel* m_contextLabel;
    QProgressBar* m_contextBar;
    QComboBox*          m_providerCombo;
    QComboBox*          m_roleCombo;
    QCheckBox*          m_autoApproveCheck;
    QPushButton*        m_scrollToBottomBtn;
    QPushButton*        m_autoScrollBtn; // Magnet button
    QPushButton*        m_stopBtn;       // Stop agent button
    QPushButton*        m_themeBtn;      // Dark/Light toggle
    QPushButton*        m_debugBtn;      // Save debug logs
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

    CodeHex::HelpDialog* m_helpDialog = nullptr;

    void updateTokenLabel(int in = -1, int out = -1);
    void updateButtonIcons();
    void onCursorBlink();
};

}  // namespace CodeHex