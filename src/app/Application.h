#pragma once
#include <QApplication>
#include <memory>

namespace CodeHex {

class AppConfig;
class SessionManager;
class ChatController;
class CliRunner;
class ScriptManager;
class AudioRecorder;
class AudioPlayer;
class MainWindow;

class Application : public QApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv);
    ~Application() override;

    int run();

private:
    void setupComponents();
    void setupCliRunner();

    std::unique_ptr<AppConfig>      m_config;
    std::unique_ptr<SessionManager> m_sessions;
    std::unique_ptr<CliRunner>      m_runner;
    std::unique_ptr<ScriptManager>  m_scripts;
    std::unique_ptr<ChatController> m_controller;
    std::unique_ptr<AudioRecorder>  m_recorder;
    std::unique_ptr<AudioPlayer>    m_player;
    std::unique_ptr<MainWindow>     m_mainWindow;
};

}  // namespace CodeHex
