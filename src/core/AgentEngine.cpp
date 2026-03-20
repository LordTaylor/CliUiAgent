#include "AgentEngine.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include "ToolExecutor.h"
#include "tools/ToolUtils.h"
#include "tools/SearchRepoTool.h"
#include "rag/EmbeddingManager.h"
#include "rag/CodebaseIndexer.h"
#include "SessionManager.h"
#include "AppConfig.h"
#include "TokenCounter.h"
#include "../cli/CliRunner.h"
#include "../data/Session.h"

namespace CodeHex {

AgentEngine::AgentEngine(AppConfig* config, 
                         SessionManager* sessions, 
                         CliRunner* runner, 
                         ToolExecutor* toolExecutor,
                         QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_sessions(sessions)
    , m_runner(runner)
    , m_toolExecutor(toolExecutor)
{
    m_currentRole = Role::Base;
    m_manualApproval = false;
    // Default Permissions - use tool.name() case as standard
    m_toolPermissions["ReadFile"]      = Permission::Allow;
    m_toolPermissions["WriteFile"]     = Permission::Allow;
    m_toolPermissions["ListDirectory"] = Permission::Allow;
    m_toolPermissions["Bash"]          = Permission::Ask;
    m_toolPermissions["SearchFiles"]   = Permission::Allow;
    m_toolPermissions["Grep"]          = Permission::Allow;
    m_toolPermissions["Replace"]       = Permission::Allow;

    m_embeddings = new EmbeddingManager(this);
    m_indexer = new CodebaseIndexer(m_embeddings, this);

    // Register tool
    m_toolExecutor->registerTool(std::make_shared<SearchRepoTool>(m_indexer));

    // Connect Runner
    connect(m_runner, &CliRunner::outputChunk,    this, &AgentEngine::onOutputChunk);
    connect(m_runner, &CliRunner::rawOutput,      this, &AgentEngine::onRawOutput);
    connect(m_runner, &CliRunner::errorChunk,     this, &AgentEngine::onErrorChunk);
    connect(m_runner, &CliRunner::toolCallReady,  this, &AgentEngine::onToolCallReady);
    connect(m_runner, &CliRunner::finished,       this, &AgentEngine::onRunnerFinished);
    
    connect(m_toolExecutor, &ToolExecutor::toolFinished, this, &AgentEngine::onToolResultReceived);
}

void AgentEngine::process(const QString& userInput, const QList<Attachment>& attachments) {
    auto session = m_sessions->currentSession();
    if (!session) return;

    // Prepare message
    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    
    userMsg.addText(userInput);
    
    userMsg.timestamp = QDateTime::currentDateTime();
    
    QStringList imagePaths;
    for (const auto& att : attachments) {
        userMsg.addAttachment(att);
        if (att.type == Attachment::Type::Image) {
            imagePaths << att.filePath;
        }
    }

    session->appendMessage(userMsg);
    session->save();

    m_currentResponse.clear();
    m_isRunning = true;
    
    // Background indexing (Codebase Awareness)
    QtConcurrent::run([this]() {
        m_indexer->indexDirectory(m_config->workingFolder());
    });
    
    emit statusChanged("Agent is thinking...");

    // Build enriched prompt
    QString enrichedPrompt = userInput; 
    
    // Update input token stats
    const int inputTokens = TokenCounter::estimate(enrichedPrompt);
    session->updateTokens(inputTokens, 0);

    m_runner->send(enrichedPrompt, m_config->workingFolder(), imagePaths, session->messages, systemPrompt());
}

void AgentEngine::stop() {
    m_runner->stop();
}

bool AgentEngine::isRunning() const {
    return m_runner->isProfileRunning();
}

void AgentEngine::onOutputChunk(const QString& chunk) {
    m_currentResponse += chunk;
    emit tokenReceived(chunk);
}

void AgentEngine::onRawOutput(const QString& raw) {
    emit consoleOutput(raw);
}

QString AgentEngine::loadRolePrompt(Role role) const {
    QString fileName;
    switch (role) {
        case Role::Base:     fileName = "base.txt"; break;
        case Role::Explorer: fileName = "explorer.txt"; break;
        case Role::Executor: fileName = "executor.txt"; break;
        case Role::Reviewer: fileName = "reviewer.txt"; break;
    }
    
    QDir projectRoot(m_config->workingFolder());
    QFile file(projectRoot.absoluteFilePath("resources/prompts/" + fileName));
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return QString();
}

QString AgentEngine::systemPrompt() const {
    QString base = loadRolePrompt(Role::Base);
    
    if (m_toolExecutor) {
        base += "\n\n" + m_toolExecutor->getToolDefinitions();
    }
    
    if (m_currentRole == Role::Base) return base;
    
    QString rolePrompt = loadRolePrompt(m_currentRole);
    return base + "\n\n" + rolePrompt;
}

void AgentEngine::onErrorChunk(const QString& chunk) {
    emit consoleOutput("[stderr] " + chunk);
}

void AgentEngine::setManualApproval(bool enabled) {
    m_manualApproval = enabled;
}

void AgentEngine::setToolPermission(const QString& toolName, Permission p) {
    m_toolPermissions[toolName] = p;
}

bool AgentEngine::isPathAllowed(const QString& path) const {
    if (path.isEmpty()) return true;
    
    QFileInfo info(path);
    QString absPath = info.absoluteFilePath();
    QString workDir = m_config->workingFolder();
    
    // Sandbox: Allow only within working directory or its subdirectories
    return absPath.startsWith(workDir);
}

AgentEngine::Permission AgentEngine::toolPermission(const QString& toolName) const {
    if (!m_manualApproval) {
        return Permission::Allow;
    }
    
    if (m_toolPermissions.contains(toolName)) {
        return m_toolPermissions.value(toolName);
    }
    
    return Permission::Ask;
}

void AgentEngine::onToolCallReady(const CodeHex::ToolCall& call) {
    if (!m_isRunning) return;

    // --- Sandbox Check ---
    if (call.input.contains("path")) {
        QString path = call.input.value("path").toString();
        if (!isPathAllowed(path)) {
            emit errorOccurred("Sandbox violation: Path is outside of working directory: " + path);
            return;
        }
    }

    Permission p = toolPermission(call.name);
    if (p == Permission::Deny) {
        emit errorOccurred("Tool execution denied: " + call.name);
        return;
    }
    
    if (p == Permission::Ask) {
        emit toolApprovalRequested(call.name, call.input);
        m_pendingCalls.append(call); // Re-added this line to maintain pending calls for approval
        return;
    }

    // Execute Tool
    emit toolCallStarted(call.name, call.input);
    if (m_syncTools) {
        m_toolExecutor->executeSync(call, m_config->workingFolder());
    } else {
        m_toolExecutor->execute(call, m_config->workingFolder());
    }
}

void AgentEngine::approveToolCall(const ToolCall& call) {
    emit statusChanged(QString("Approved: Executing %1...").arg(call.name));
    emit toolCallStarted(call.name, call.input); // Emit started if it was waiting
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void AgentEngine::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName);
    
    auto* session = m_sessions->currentSession();
    if (!session) {
        return;
    }

    // Convert ToolResult to a Message (normally this is handled as part of the tool turn)
    Message toolMsg;
    toolMsg.id = QUuid::createUuid();
    toolMsg.role = Message::Role::User; // In some APIs this is Role::Tool
    
    CodeBlock block;
    block.type = BlockType::Text; // Or a specific 'Result' type if available
    block.content = result.content;
    toolMsg.contentBlocks << block;
    toolMsg.timestamp = QDateTime::currentDateTime();

    session->appendMessage(toolMsg);
    session->save();

    if (!m_runner->isProfileRunning()) {
        toolMsg.addText(result.content);
        toolMsg.toolResults << result; 
        session->appendMessage(toolMsg);

        if (!result.isError) {
            QString sp = systemPrompt();
            if (m_runner) {
                m_runner->send("", m_config->workingFolder(), {}, session->messages, sp);
            }
        }
    }
}

void AgentEngine::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode);
    m_isRunning = false;
    if (m_currentResponse.trimmed().isEmpty()) {
        emit statusChanged("");
        return;
    }

    buildAssistantMessage(m_currentResponse);

    // Parse LLM-agnostic XML tool calls
    QRegularExpression re("<tool_call>\\s*<name>([^<]+)</name>\\s*<input>(.*?)</input>\\s*</tool_call>", 
                          QRegularExpression::DotMatchesEverythingOption | QRegularExpression::InvertedGreedinessOption);
    
    QRegularExpressionMatchIterator i = re.globalMatch(m_currentResponse);
    QList<ToolCall> parsedCalls;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        ToolCall call;
        call.id = QUuid::createUuid().toString();
        call.name = match.captured(1).trimmed();
        
        QString jsonStr = match.captured(2).trimmed();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (!doc.isNull() && doc.isObject()) {
            call.input = doc.object();
            parsedCalls.append(call);
        } else {
            qWarning() << "AgentEngine: Failed to parse JSON from XML tool:" << err.errorString() << "\nJSON:\n" << jsonStr;
        }
    }

    // Fallback: Parse standard Markdown Bash blocks if no explicit tools found
    // This perfectly supports Faza 1 behavior for generic OSS coding models
    if (parsedCalls.isEmpty()) {
        QRegularExpression bashRe("```bash\\n(.*?)```", 
                              QRegularExpression::DotMatchesEverythingOption | QRegularExpression::InvertedGreedinessOption);
        QRegularExpressionMatchIterator bashIter = bashRe.globalMatch(m_currentResponse);
        while (bashIter.hasNext()) {
            QRegularExpressionMatch match = bashIter.next();
            ToolCall call;
            call.id = QUuid::createUuid().toString();
            call.name = "Bash";
            QJsonObject input;
            input["command"] = match.captured(1).trimmed();
            call.input = input;
            parsedCalls.append(call);
        }
    }

    if (!parsedCalls.isEmpty()) {
        onToolCallReady(parsedCalls.first());
    } else {
        emit statusChanged("");
    }
}


void AgentEngine::buildAssistantMessage(const QString& plainText) {
    auto* session = m_sessions->currentSession();
    if (!session) return;

    Message msg;
    msg.id = QUuid::createUuid();
    msg.role = Message::Role::Assistant;
    msg.timestamp = QDateTime::currentDateTime();

    CodeBlock block;
    block.type = BlockType::Text;
    block.content = plainText;
    msg.contentBlocks << block;

    // Handle session auto-rename if first message
    if (session->messages.size() <= 2) { 
        QString title = plainText.section(QRegularExpression("[.!?]"), 0, 0).trimmed();
        if (title.length() > 40) title = title.left(37) + "...";
        if (!title.isEmpty()) {
            session->title = title;
        }
    }

    // Try to separate Thought from Result if tags are present
    QString content = plainText;
    if (content.contains("<thought>") && content.contains("</thought>")) {
        int start = content.indexOf("<thought>") + 9;
        int end = content.indexOf("</thought>");
        QString thought = content.mid(start, end - start).trimmed();
        QString remaining = content.mid(end + 10).trimmed();

        if (!thought.isEmpty()) {
            CodeBlock thoughtBlock;
            thoughtBlock.type = BlockType::Thinking;
            thoughtBlock.content = thought;
            msg.contentTypes << Message::ContentType::Thinking;
            msg.contentBlocks << thoughtBlock;
        }
        
        if (!remaining.isEmpty()) {
            CodeBlock textBlock;
            textBlock.type = BlockType::Text;
            textBlock.content = remaining;
            msg.contentTypes << Message::ContentType::Text;
            msg.contentBlocks << textBlock;
        }
    } else {
        CodeBlock block;
        block.type = BlockType::Text;
        block.content = plainText;
        msg.contentBlocks << block;
        msg.contentTypes << Message::ContentType::Text;
    }

    session->appendMessage(msg);
    session->save();
    
    emit responseComplete(msg);
    emit statusChanged("");
}

} // namespace CodeHex
