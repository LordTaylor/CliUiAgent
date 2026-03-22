#include "Application.h"
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QPixmap>
#include "../audio/AudioPlayer.h"
#include "../audio/AudioRecorder.h"
#include "../cli/CliRunner.h"
#include "../cli/ConfigurableProfile.h"
#include "../cli/OllamaProfile.h"
#include "../core/AppConfig.h"
#include "../core/ChatController.h"
#include "../core/SessionManager.h"
#include "../scripting/ScriptManager.h"
#include "../ui/MainWindow.h"
#include "../core/TokenCounter.h"
#include "../core/CrashHandler.h"
#include "../core/AgentEngine.h"

namespace CodeHex {

Application::Application(int& argc, char** argv) : QApplication(argc, argv) {
    setApplicationName("CodeHex");
    setApplicationVersion("0.1.0");
    setOrganizationName("CodeHex");

    // Set application icon for Windows / Linux (macOS uses app.icns from the bundle)
#ifndef Q_OS_MACOS
    QIcon appIcon;
    appIcon.addPixmap(QPixmap(":/resources/icons/app_16.png"),  QIcon::Normal, QIcon::Off);
    appIcon.addPixmap(QPixmap(":/resources/icons/app_32.png"),  QIcon::Normal, QIcon::Off);
    appIcon.addPixmap(QPixmap(":/resources/icons/app_48.png"),  QIcon::Normal, QIcon::Off);
    appIcon.addPixmap(QPixmap(":/resources/icons/app_128.png"), QIcon::Normal, QIcon::Off);
    appIcon.addPixmap(QPixmap(":/resources/icons/app_256.png"), QIcon::Normal, QIcon::Off);
    setWindowIcon(appIcon);
#else
    setWindowIcon(QIcon(":/resources/icons/app.png"));
#endif
}

Application::~Application() {
    if (m_controller && m_controller->agent()) {
        m_controller->agent()->savePersistence();
    }
}

int Application::run() {
    setupComponents();
    m_mainWindow->show();
    return exec();
}

void Application::setupComponents() {
    CrashHandler::init();
    TokenCounter::init();

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
    
    m_mainWindow = std::make_unique<MainWindow>(
        m_config.get(),
        m_sessions.get(),
        m_controller.get(),
        m_recorder.get(),
        m_player.get(),
        m_extraProfiles);


    // --- Persistence (Roadmap #9 & #10) ---
    m_controller->agent()->loadPersistence();

    // When the user switches provider in MainWindow, re-configure CliRunner
    connect(m_config.get(), &AppConfig::activeProviderChanged,
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
    const QString profileId = m_config->activeProviderId();

    // 1. Check extra (configurable) profiles first
    for (const auto& entry : m_extraProfiles) {
        if (entry.name == profileId) {
            auto p = ConfigurableProfile::fromFile(entry.filePath);
            if (p) {
                m_runner->setProfile(std::move(p));
                return;
            }
        }
    }

    // 2. Use active provider from config
    const auto provider = m_config->activeProvider();
    if (!provider.id.isEmpty()) {
        m_runner->setProfile(ConfigurableProfile::fromProvider(provider));
        
        // --- macOS Path Fix for Ollama.app ---
#ifdef Q_OS_MAC
        {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            QString path = env.value("PATH");
            QString ollamaRes = "/Applications/Ollama.app/Contents/Resources";
            if (!path.contains(ollamaRes)) {
                path = ollamaRes + ":" + path;
                env.insert("PATH", path);
            }
        }
#endif
        return;
    }

    // 3. Absolute fallback to a basic profile that uses curl (robust)
    m_runner->setProfile(std::make_unique<ConfigurableProfile>());
}

}  // namespace CodeHex
