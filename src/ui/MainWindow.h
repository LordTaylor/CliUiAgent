#pragma once
#include <QComboBox>
#include <QMainWindow>
#include <QSplitter>
#include "../data/Attachment.h"
#include "../data/Message.h"
#include "../data/Session.h"

namespace CodeHex {

class AppConfig;
class SessionManager;
class ChatController;
class AudioRecorder;
class AudioPlayer;
class ScriptManager;

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

private:
    void setupUi();
    void loadStyleSheet();
    void switchSession(Session* session);
    void populateProfileCombo();

    AppConfig* m_config;
    SessionManager* m_sessions;
    ChatController* m_controller;
    AudioRecorder* m_recorder;
    AudioPlayer* m_player;

    // UI widgets
    QSplitter* m_splitter;
    SessionPanel* m_sessionPanel;
    ChatView* m_chatView;
    MessageModel* m_messageModel;
    InputPanel* m_inputPanel;
    ConsoleWidget* m_console;
    WorkFolderSelector* m_folderSelector;
    QComboBox* m_profileCombo;

    // Streaming state: accumulates tokens into a live bubble
    QString m_streamingText;
    bool m_hasStreamingMsg = false;
};

}  // namespace CodeHex
