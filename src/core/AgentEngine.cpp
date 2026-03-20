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

    if (m_isRunning) {
        qDebug() << "[AgentEngine] Busy. Enqueuing request.";
        m_requestQueue.enqueue({userInput, attachments});
        emit statusChanged(QString("Request queued (%1 in queue)").arg(m_requestQueue.size()));
        return;
    }

    // Safety guard: Don't append if ID exists (prevents UI sync duplicates)
    bool exists = false;
    for (const auto& m : session->messages) {
        if (m.id == userMsg.id) { exists = true; break; }
    }
    if (!exists) {
        session->appendMessage(userMsg);
    }
    session->save();

    m_currentResponse.clear();
    m_isThinkingStream = false;
    m_thoughtBuffer.clear();
    m_isRunning = true;
    
    // Background indexing (Codebase Awareness)
    QtConcurrent::run([this]() {
        m_indexer->indexDirectory(m_config->workingFolder());
    });
    
    emit statusChanged("Agent is thinking...");

    // Build enriched prompt (Auto-RAG)
    emit statusChanged("🔍 Analyzing codebase for context...");
    injectAutoContext(userInput);
    
    QString enrichedPrompt = userInput; 
    
    // Update input token stats
    const int inputTokens = TokenCounter::estimate(enrichedPrompt);
    session->updateTokens(inputTokens, 0);

    emit statusChanged("🧠 Thinking...");
    
    // Apply dynamic model selection if set
    if (!m_selectedModel.isEmpty() && m_runner->profile()) {
        m_runner->profile()->setModel(m_selectedModel);
    }
    
    m_runner->send(enrichedPrompt, m_config->workingFolder(), imagePaths, session->messages, systemPrompt());
}

void AgentEngine::injectAutoContext(const QString& query) {
    m_autoContext.clear();
    auto chunks = m_indexer->search(query, 3); // Top 3 snippets
    if (chunks.isEmpty()) return;

    m_autoContext = "\n\n### AUTOMATIC PROJECTS CONTEXT (Based on semantic search):\n";
    for (const auto& chunk : chunks) {
        m_autoContext += QString("--- File: %1 (Lines %2-%3) ---\n%4\n")
                            .arg(chunk.filePath)
                            .arg(chunk.startLine)
                            .arg(chunk.endLine)
                            .arg(chunk.content);
    }
}

void AgentEngine::stop() {
    m_isRunning = false;
    m_runner->stop();
    m_requestQueue.clear();
}

void AgentEngine::reset() {
    stop();
    m_pendingCalls.clear();
    m_currentResponse.clear();
    m_isThinkingStream = false;
    m_thoughtBuffer.clear();
}

bool AgentEngine::isRunning() const {
    return m_isRunning || m_runner->isProfileRunning();
}

void AgentEngine::onOutputChunk(const QString& chunk) {
    m_currentResponse += chunk;

    // Detect tag start
    if (!m_isThinkingStream && m_currentResponse.contains("<thought>")) {
        m_isThinkingStream = true;
        m_thoughtBuffer.clear(); // Start fresh thought tracking
        emit statusChanged("🧠 Reasoning...");
        return; 
    }
    
    // While in thinking mode, suppress tokens and update status banner
    if (m_isThinkingStream) {
        m_thoughtBuffer += chunk;
        if (m_currentResponse.contains("</thought>")) {
            m_isThinkingStream = false;
            emit statusChanged("💡 Thought finalized.");
        } else {
            // Show a snippet of the current thought in the banner
            QString glimpse = m_thoughtBuffer.trimmed();
            if (glimpse.length() > 50) glimpse = "..." + glimpse.right(47);
            emit statusChanged("🧠 " + glimpse);
        }
        return;
    }

    // Suppress tool tags too
    if (m_currentResponse.contains("<name>") || m_currentResponse.contains("<input>")) {
        return; 
    }

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
    
    // Load from Qt Resources instead of working directory
    QFile file(":/resources/prompts/" + fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    qWarning() << "[AgentEngine] Failed to load prompt from resource:" << file.fileName();
    return QString();
}

QString AgentEngine::systemPrompt() const {
    QString base = loadRolePrompt(m_currentRole);
    if (m_currentRole != Role::Base) {
        // Optionally append Base rules if the specific role prompt is too narrow
        // but for now, let's assume each role prompt is self-contained or 
        // we append the base rules explicitly if needed.
    }
    
    // 🧠 AGENT BRAIN: Load persistent memory
    QString memoryPath = m_config->workingFolder() + "/.agent/memory.md";
    QFile memFile(memoryPath);
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        base += "\n\n### PROJECT MEMORY (Read to avoid repeating errors):\n" + 
                QString::fromUtf8(memFile.readAll());
    }

    // 📜 PROJECT RULES: Load mandatory constraints
    QString rulesPath = m_config->workingFolder() + "/.agent/rules.md";
    QFile rulesFile(rulesPath);
    if (rulesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        base += "\n\n### MANDATORY PROJECT RULES & GUIDELINES (STRICT ADHERENCE REQUIRED):\n" + 
                QString::fromUtf8(rulesFile.readAll());
    }

    base += "\n\n### INTERNAL TOOLS & SCRATCHPAD:\n"
            "- You have a dedicated directory `.agent/scratchpad/` for internal scripts.\n"
            "- If you encounter a complex task that standard tools can't solve easily, "
            "WRITE a custom script (Python/Bash) into that folder and EXECUTE it.\n"
            "- This is for your own automation and data processing needs.\n"
            "- 🍏 **MacOS:** Use `osascript -e 'tell app \"Terminal\" to do script \"python3 %1\"'` to open a real window.\n"
            "- 🪟 **Windows:** Use `cmd /c start powershell -Command \"python %1; Read-Host 'Press Enter to exit'\"`.\n"
            "- 🐧 **Linux:** Use `x-terminal-emulator -e \"bash -c 'python3 %1; read -p \\\"Press Enter to exit\\\"'\"`.\n"
            "You CAN open windows on the user's computer.";

    if (m_toolExecutor) {
        base += "\n\n" + m_toolExecutor->getToolDefinitions();
    }

    if (!m_autoContext.isEmpty()) {
        base += m_autoContext;
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
    
    QString workDir = m_config->workingFolder();
    
    // Resolve relative paths against working directory, NOT process CWD
    QFileInfo info(QDir(workDir), path);
    QString absPath = info.absoluteFilePath();
    
    qDebug() << "[AgentEngine] isPathAllowed: path=" << path << " resolved=" << absPath << " workDir=" << workDir;
    
    // Sandbox: Allow only within working directory or its subdirectories
    bool allowed = absPath.startsWith(workDir);
    if (!allowed) {
        qWarning() << "[AgentEngine] SANDBOX BLOCKED: " << absPath << " is outside " << workDir;
    }
    return allowed;
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
    qDebug() << "[AgentEngine] onToolCallReady: tool=" << call.name << " m_isRunning=" << m_isRunning;
    // NOTE: Do NOT check m_isRunning here — onRunnerFinished sets it to false
    // before calling this method. The guard was silently blocking ALL tool executions.

    // --- Sandbox Check ---
    auto* session = m_sessions->currentSession();
    if (!session) return;

    if (call.input.contains("path")) {
        QString path = call.input.value("path").toString();
        if (!isPathAllowed(path)) {
            qWarning() << "[AgentEngine] Sandbox violation for path:" << path;
            emit errorOccurred("Sandbox violation: Path is outside of working directory: " + path);
            return;
        }
    }

    Permission p = toolPermission(call.name);
    qDebug() << "[AgentEngine] Permission for" << call.name << "=" << (int)p << " (0=Allow,1=Ask,2=Deny)";
    if (p == Permission::Deny) {
        emit errorOccurred("Tool execution denied: " + call.name);
        return;
    }
    
    if (p == Permission::Ask) {
        qDebug() << "[AgentEngine] Requesting approval for" << call.name;
        emit toolApprovalRequested(call.name, call.input);
        m_pendingCalls.append(call);
        return;
    }

    // Execute Tool
    qDebug() << "[AgentEngine] EXECUTING tool:" << call.name;
    
    // Add compact CALL log to chat
    Message callMsg;
    callMsg.id = QUuid::createUuid();
    callMsg.role = Message::Role::Assistant;
    CodeBlock callBlock;
    callBlock.type = BlockType::LogStep;
    callBlock.content = "⚙ CALL: " + call.name;
    callMsg.contentBlocks << callBlock;
    callMsg.timestamp = QDateTime::currentDateTime();
    session->appendMessage(callMsg);
    session->save();

    emit toolCallStarted(call.name, call.input);
    if (m_syncTools) {
        m_toolExecutor->executeSync(call, m_config->workingFolder());
    } else {
        m_toolExecutor->execute(call, m_config->workingFolder());
    }
}

void AgentEngine::approveToolCall(const ToolCall& call) {
    auto* session = m_sessions->currentSession();
    if (session) {
        Message callMsg;
        callMsg.id = QUuid::createUuid();
        callMsg.role = Message::Role::Assistant;
        CodeBlock callBlock;
        callBlock.type = BlockType::LogStep;
        callBlock.content = "⚙ CALL: " + call.name;
        callMsg.contentBlocks << callBlock;
        callMsg.timestamp = QDateTime::currentDateTime();
        session->appendMessage(callMsg);
        session->save();
    }

    emit statusChanged(QString("Approved: Executing %1...").arg(call.name));
    emit toolCallStarted(call.name, call.input); // Emit started if it was waiting
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void AgentEngine::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName);
    
    auto* session = m_sessions->currentSession();
    if (!session) return;

    // Create a NEW message for the Tool Result
    Message toolMsg;
    toolMsg.id = QUuid::createUuid();
    toolMsg.role = Message::Role::User; // Fallback for local models
    toolMsg.timestamp = QDateTime::currentDateTime();
    
    // Add the tool result data
    toolMsg.toolResults << result;
    
    // Add text block for display (Compact LogStep)
    CodeBlock block;
    block.type = BlockType::LogStep;
    block.content = (result.isError ? "❌ FAILED: " : "✅ DONE: ") + toolName;
    toolMsg.contentBlocks << block;
    toolMsg.addText(result.content); // Full result is still kept in the hidden text for model context/fallback

    session->appendMessage(toolMsg);
    session->save();

    // Automatically trigger the next agent turn if the profile isn't already doing something
    if (!m_runner->isProfileRunning()) {
        if (!result.isError) {
            QString sp = systemPrompt();
            if (m_runner) {
                // For local models, a small nudge helps prevent loops or empty responses
                m_runner->send("Tool executed successfully. Please continue or finalize the task.", 
                              m_config->workingFolder(), {}, session->messages, sp);
            }
        } else {
            // If it was an error, just notify the user/model without automatic continue if preferred
            // but usually agent should try to fix the error.
             QString sp = systemPrompt();
             m_runner->send("Tool execution failed. Analyze the error and try a different approach.", 
                           m_config->workingFolder(), {}, session->messages, sp);
        }
    }
}

void AgentEngine::onRunnerFinished(int exitCode) {
    Q_UNUSED(exitCode);
    m_isRunning = false;
    QString lastAssistantContent;
    auto session = m_sessions->currentSession();
    if (session && !session->messages.isEmpty()) {
        for (int i = session->messages.size() - 1; i >= 0; --i) {
            if (session->messages[i].role == Message::Role::Assistant) {
                lastAssistantContent = session->messages[i].textFromContentBlocks().trimmed();
                break;
            }
        }
    }

    if (!lastAssistantContent.isEmpty() && m_currentResponse.trimmed() == lastAssistantContent) {
        qWarning() << "[AgentEngine] LOOP DETECTED. Assistant repeated itself exactly.";
        emit statusChanged("Loop detected. Nudging agent...");
        
        // Don't append the duplicate message. Instead, send a nudge.
        m_isRunning = true;
        m_runner->send("WARNING: You just sent the EXACT SAME response. DO NOT repeat yourself. If the task is done, say so. If not, take a NEW action or provide a NEW thought.", 
                      m_config->workingFolder(), {}, session->messages, systemPrompt());
        return;
    }

    buildAssistantMessage(m_currentResponse);

    // 1. Optimized XML Parser (Lax mode for local LLMs)
    // We look for <name> and <input> blocks. They may or may not be inside <tool_call>.
    // Using a more robust regex that ignores leading/trailing whitespace and block tags.
    QRegularExpression re("<name>\\s*([^<\\s]+)\\s*</name>\\s*<input>\\s*(.*?)\\s*</input>", 
                          QRegularExpression::DotMatchesEverythingOption);
    
    QRegularExpressionMatchIterator i = re.globalMatch(m_currentResponse);
    QList<ToolCall> parsedCalls;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString tname = match.captured(1).trimmed();
        QString jsonStr = match.captured(2).trimmed();
        
        qDebug() << "[AgentEngine] Found potential tool call:" << tname << "with input length:" << jsonStr.length();

        ToolCall call;
        call.id = QUuid::createUuid().toString();
        call.name = tname;
        
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (!doc.isNull() && doc.isObject()) {
            call.input = doc.object();
            parsedCalls.append(call);
            qDebug() << "[AgentEngine] Successfully parsed tool:" << tname;
            break; // Parse only the first valid one
        } else {
            qWarning() << "[AgentEngine] FAILED to parse JSON for tool" << tname << ":" << err.errorString();
            qDebug() << "[AgentEngine] Bad JSON snippet:" << jsonStr.left(100) << "...";
        }
    }

    // Fallback: Parse standard Markdown Bash blocks if no explicit tools found
    // This perfectly supports Faza 1 behavior for generic OSS coding models
    if (parsedCalls.isEmpty()) {
        QRegularExpression bashRe("```bash\\n(.*?)```", 
                              QRegularExpression::DotMatchesEverythingOption);
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
            break; // Parse only the first Bash block
        }
    }

    qDebug() << "[AgentEngine] onRunnerFinished: parsedCalls.size()=" << parsedCalls.size();
    if (!parsedCalls.isEmpty()) {
        qDebug() << "[AgentEngine] Dispatching tool:" << parsedCalls.first().name;
        onToolCallReady(parsedCalls.first());
    } else {
        qDebug() << "[AgentEngine] No tool calls found in response";
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

    // 1. Extract ALL <thought> blocks
    QRegularExpression thoughtRe("<thought>(.*?)</thought>", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = thoughtRe.globalMatch(plainText);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString thought = match.captured(1).trimmed();
        if (!thought.isEmpty()) {
            CodeBlock b;
            b.type = BlockType::Thinking;
            b.content = thought;
            msg.contentBlocks << b;
            msg.contentTypes << Message::ContentType::Thinking;
        }
    }

    // 2. Extract Clean Text (stripping all internal XML tags)
    QString cleanText = plainText;
    cleanText.remove(QRegularExpression("<thought>.*?</thought>", QRegularExpression::DotMatchesEverythingOption));
    cleanText.remove(QRegularExpression("<name>.*?</name>", QRegularExpression::DotMatchesEverythingOption));
    cleanText.remove(QRegularExpression("<input>.*?</input>", QRegularExpression::DotMatchesEverythingOption));
    cleanText.remove("<tool_call>");
    cleanText.remove("</tool_call>");
    cleanText = cleanText.trimmed();

    if (!cleanText.isEmpty()) {
        CodeBlock b;
        b.type = BlockType::Text;
        b.content = cleanText;
        msg.contentBlocks << b;
        msg.contentTypes << Message::ContentType::Text;
    }

    // 3. Handle session auto-rename if first message
    if (session->messages.size() <= 2) { 
        QString title = cleanText.section(QRegularExpression("[.!?]"), 0, 0).trimmed();
        if (title.length() > 40) title = title.left(37) + "...";
        if (title.isEmpty()) title = "New Task";
        session->title = title;
    }

    session->appendMessage(msg);
    emit responseComplete(msg);
    emit statusChanged("");
}

void AgentEngine::processNextQueueItem() {
    if (m_requestQueue.isEmpty()) return;
    
    PendingRequest next = m_requestQueue.dequeue();
    qDebug() << "[AgentEngine] Processing next queued request.";
    process(next.input, next.attachments);
}

} // namespace CodeHex
