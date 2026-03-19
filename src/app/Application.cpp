#include "Application.h"
#include <QDebug>
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../cli/ClaudeProfile.h"
#include "../cli/CliRunner.h"
#include "../cli/GptProfile.h"
#include "../cli/OllamaProfile.h"
#include "../core/AppConfig.h"
#include "../core/ChatController.h"
#include "../core/SessionManager.h"
#include "../scripting/ScriptManager.h"
#include "../ui/MainWindow.h"

namespace CodeHex {

Application::Application(int& argc, char** argv) : QApplication(argc, argv) {
    setApplicationName("CodeHex");
    setApplicationVersion("0.1.0");
    setOrganizationName("CodeHex");
}

Application::~Application() = default;

int Application::run() {
    setupComponents();
    m_mainWindow->show();
    return exec();
}

void Application::setupComponents() {
    // Config
    m_config = std::make_unique<AppConfig>();
    m_config->ensureDirectories();
    m_config->load();

    // Sessions
    m_sessions = std::make_unique<SessionManager>(m_config.get());

    // CLI runner
    m_runner = std::make_unique<CliRunner>();
    setupCliRunner();

    // Scripts
    m_scripts = std::make_unique<ScriptManager>(
        m_config->luaScriptsDir(),
        m_config->pythonScriptsDir());
    if (!m_scripts->initialize()) {
        qWarning() << "Script engine initialization partially failed.";
    }
    m_scripts->loadAll();

    // Chat controller
    m_controller = std::make_unique<ChatController>(
        m_config.get(),
        m_sessions.get(),
        m_runner.get(),
        m_scripts.get());

    // Audio
    m_recorder = std::make_unique<AudioRecorder>();
    m_player   = std::make_unique<AudioPlayer>();

    // Main window
    m_mainWindow = std::make_unique<MainWindow>(
        m_config.get(),
        m_sessions.get(),
        m_controller.get(),
        m_recorder.get(),
        m_player.get());

    // React to profile changes from MainWindow
    // (profile switch is triggered by combo box which calls config->setActiveProfile)
    connect(m_config.get(), &AppConfig::destroyed, this, []{});  // placeholder hook
}

void Application::setupCliRunner() {
    const QString profile = m_config->activeProfile();
    std::unique_ptr<CliProfile> p;
    if (profile == "ollama") {
        p = std::make_unique<OllamaProfile>();
    } else if (profile == "gpt") {
        p = std::make_unique<GptProfile>();
    } else {
        p = std::make_unique<ClaudeProfile>();
    }
    m_runner->setProfile(std::move(p));
}

}  // namespace CodeHex
