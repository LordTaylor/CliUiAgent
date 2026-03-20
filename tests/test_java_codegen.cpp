#include <catch2/catch_test_macros.hpp>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QProcess>
#include "AgentEngine.h"
#include "CliRunner.h"
#include "ConfigurableProfile.h"
#include "AppConfig.h"
#include "SessionManager.h"
#include "ToolExecutor.h"

using namespace CodeHex;

/**
 * Realistic Integration Test:
 * 1. Ask agent to create Java HelloWorld
 * 2. Verify file creation on disk
 * 3. Verify documentation creation
 */
TEST_CASE("Agent generates Java HelloWorld and documentation", "[Integration][Java]") {
    QTemporaryDir dataDir;
    AppConfig config;
    config.setDataDir(dataDir.path());
    config.ensureDirectories();

    QTemporaryDir workDir;
    // Explicitly ensure workDir is clean (though QTemporaryDir is fresh by default)
    if (QDir(workDir.path()).exists()) {
        QDir(workDir.path()).removeRecursively();
    }
    QDir().mkpath(workDir.path());
    config.setWorkingFolder(workDir.path());

    // Create a mock profile file
    QString profilePath = dataDir.path() + "/profiles/java_test.json";
    QDir().mkpath(dataDir.path() + "/profiles");
    QFile profileFile(profilePath);
    if (profileFile.open(QIODevice::WriteOnly)) {
        QJsonObject profileObj;
        profileObj["type"] = "openai-compatible";
        profileObj["name"] = "java_test";
        profileObj["displayName"] = "Java Test Profile";
        profileObj["baseUrl"] = "http://localhost:1234/v1";
        profileObj["apiKey"] = "lm-studio";
        profileObj["model"] = "qwen/qwen2.5-coder-14b";
        profileFile.write(QJsonDocument(profileObj).toJson());
        profileFile.close();
    }

    SessionManager sessions(&config);
    CliRunner runner;
    // CRITICAL: Set the profile on the runner, otherwise it won't send anything!
    auto profile = ConfigurableProfile::fromFile(profilePath);
    REQUIRE(profile != nullptr);
    runner.setProfile(std::move(profile));

    ToolExecutor toolExecutor(nullptr);
    AgentEngine engine(&config, &sessions, &runner, &toolExecutor);

    // Initial session setup
    sessions.createSession("java_test", "Java Codegen Test");
    
    // Check if LM Studio is reachable
    QProcess check;
    check.start("curl", {"-s", "http://localhost:1234/v1/models"});
    if (!check.waitForFinished(2000) || check.exitCode() != 0) {
        WARN("LM Studio not reachable at localhost:1234 - skipping Java test");
        return;
    }

    QEventLoop loop;
    bool success = false;
    
    // We wait for a response that doesn't trigger more tools
    QObject::connect(&engine, &AgentEngine::responseComplete, [&](const Message& msg) {
        QString text = msg.textFromContentBlocks();
        // If the response has no tool tags, the agent is likely done with its plan
        if (!text.contains("<tool_call>") && !text.contains("<name>") && !text.contains("```bash")) {
            success = true;
            loop.quit();
        }
    });

    QObject::connect(&engine, &AgentEngine::errorOccurred, [&](const QString& err) {
        FAIL("Agent error: " + err.toStdString());
        loop.quit();
    });

    QTimer::singleShot(180000, &loop, &QEventLoop::quit); // 3 min timeout

    engine.process("Utwórz prostą aplikację Java HelloWorld, zapisz ją w folderze 'test_java' i napisz krótką dokumentację w README.md.");
    loop.exec();

    REQUIRE(success);
    
    QDir resultDir(workDir.path() + "/test_java");
    CHECK(resultDir.exists());
    CHECK(QFile::exists(workDir.path() + "/test_java/HelloWorld.java"));
    CHECK(QFile::exists(workDir.path() + "/test_java/README.md"));
}
