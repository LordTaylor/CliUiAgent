#include "Application.h"
#include <QDebug>
#include <QDir>
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../cli/ClaudeProfile.h"
#include "../cli/CliRunner.h"
#include "../cli/ConfigurableProfile.h"
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

    // Discover configurable profiles from ~/.codehex/profiles/
    discoverProfiles();

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

    // Main window — pass discovered profile entries for the combo box
    m_mainWindow = std::make_unique<MainWindow>(
        m_config.get(),
        m_sessions.get(),
        m_controller.get(),
        m_recorder.get(),
        m_player.get(),
        m_extraProfiles);

    // When the user switches profile in MainWindow, re-configure CliRunner
    connect(m_config.get(), &AppConfig::activeProfileChanged,
            this, &Application::setupCliRunner);
}

void Application::discoverProfiles() {
    m_extraProfiles.clear();
    QDir dir(m_config->profilesDir());
    const auto files = dir.entryInfoList({"*.json"}, QDir::Files, QDir::Name);
    for (const QFileInfo& fi : files) {
        auto p = ConfigurableProfile::fromFile(fi.absoluteFilePath());
        if (!p) {
            qWarning() << "ConfigurableProfile: failed to parse" << fi.fileName();
            continue;
        }
        qDebug() << "Loaded profile:" << p->displayName();
        m_extraProfiles.append({p->name(), p->displayName(), fi.absoluteFilePath()});
    }
}

void Application::setupCliRunner() {
    const QString profile = m_config->activeProfile();

    // Check extra (configurable) profiles first
    for (const auto& entry : m_extraProfiles) {
        if (entry.name == profile) {
            auto p = ConfigurableProfile::fromFile(entry.filePath);
            if (p) {
                m_runner->setProfile(std::move(p));
                return;
            }
        }
    }

    // Built-in profiles
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
