#pragma once
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QTimer>
#include "../cli/ProfileEntry.h"
#include "../data/Attachment.h"
#include "../data/Message.h"
#include "../data/Session.h"
#include "help/HelpDialog.h"

namespace CodeHex {

class AppConfig;
class SessionManager;
class ChatController;
class AudioRecorder;
class AudioPlayer;

class ChatView;
class MessageModel;
class InputPanel;
class ConsoleWidget;
class WorkFolderSelector;
class SessionPanel;

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
    void onProfileChanged(int index);
    void onHelpRequested(const QString& page = "getting-started");
    void onAbout();

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
    QSplitter*          m_splitter;
    SessionPanel*       m_sessionPanel;
    ChatView*           m_chatView;
    MessageModel*       m_messageModel;
    InputPanel*         m_inputPanel;
    ConsoleWidget*      m_console;
    WorkFolderSelector* m_folderSelector;
    QComboBox*          m_profileCombo;
    QLabel*             m_tokenLabel   = nullptr;

    // Streaming state
    QString m_streamingText;
    bool    m_hasStreamingMsg  = false;
    bool    m_isStreaming       = false;
    bool    m_cursorVisible     = false;
    QTimer* m_cursorTimer       = nullptr;

    HelpDialog* m_helpDialog = nullptr;

    void updateTokenLabel();
    void onCursorBlink();
};

}  // namespace CodeHex
