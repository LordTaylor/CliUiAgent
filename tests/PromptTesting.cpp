#include <QtTest>
#include "../src/core/PromptManager.h"
#include "../src/core/AppConfig.h"
#include "../src/core/AgentRole.h"
#include "../src/data/Message.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

using namespace CodeHex;

class PromptTesting : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_config = new AppConfig(this);
        m_promptManager = new PromptManager(m_config, this);
    }

    void testSystemPromptConstruction() {
        QString prompt = m_promptManager->buildSystemPrompt(AgentRole::Executor, "Test Context", {}, {});
        QVERIFY(prompt.contains("EXECUTOR"));
        QVERIFY(prompt.contains("Test Context"));
        QVERIFY(prompt.contains("Working Directory"));
    }

    void testRequestJsonStructure() {
        QList<Message> history;
        Message m1;
        m1.role = Message::Role::User;
        m1.addText("Hello");
        history.append(m1);

        QJsonArray tools; // Empty tools for now
        
        ContextManager::ContextStats stats;
        QJsonObject request = m_promptManager->buildRequestJson(
            AgentRole::Executor,
            "Do something",
            history,
            tools,
            {},
            {},
            16000,
            false,
            "",
            &stats
        );

        QVERIFY(request.contains("system"));
        QVERIFY(request.contains("messages"));
        QVERIFY(request.contains("tools"));
        
        QJsonArray messages = request["messages"].toArray();
        QVERIFY(messages.size() >= 2); // History + current input
        
        QJsonObject lastMsg = messages.last().toObject();
        QVERIFY(lastMsg["role"] == "user");
        QVERIFY(lastMsg["content"].toArray().first().toObject()["text"] == "Do something");
        
        QVERIFY(stats.totalTokens > 0);
    }

    void testPruningIntegration() {
        QList<Message> history;
        for (int i = 0; i < 50; ++i) {
            Message m;
            m.role = (i % 2 == 0) ? Message::Role::User : Message::Role::Assistant;
            // ~2.5k tokens per message (10k chars approx)
            m.addText(QString("Message %1 content: ").arg(i) + QString(10000, 'A'));
            history.append(m);
        }

        ContextManager::ContextStats stats;
        // Limit to a small budget to force pruning
        QJsonObject request = m_promptManager->buildRequestJson(
            AgentRole::Base,
            "Latest query",
            history,
            {},
            {},
            {},
            16000,
            false,
            "",
            &stats
        );
        
        QVERIFY(stats.usagePercentage <= 1.0f);
        QVERIFY(request["messages"].toArray().size() < 51); // 50 history + 1 current
    }

private:
    AppConfig* m_config;
    PromptManager* m_promptManager;
};

QTEST_MAIN(PromptTesting)
#include "PromptTesting.moc"
