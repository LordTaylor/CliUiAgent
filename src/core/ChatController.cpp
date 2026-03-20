#include "ChatController.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include "../cli/CliRunner.h"
#include "../scripting/ScriptManager.h"
#include "AppConfig.h"
#include "SessionManager.h"
#include "TokenCounter.h"
#include "ToolExecutor.h" // Already present

namespace CodeHex {

ChatController::ChatController(AppConfig* config,
                               SessionManager* sessions,
                               CliRunner* runner,
                               ScriptManager* scripts,
                               QObject* parent)
    : QObject(parent),
      m_config(config),
      m_sessions(sessions),
      m_runner(runner),
      m_scripts(scripts),
      m_toolExecutor(new ToolExecutor(this)) {
    connect(m_runner, &CliRunner::outputChunk,   this, &ChatController::onOutputChunk);
    connect(m_runner, &CliRunner::rawOutput,     this, &ChatController::onRawOutput);
    connect(m_runner, &CliRunner::errorChunk,    this, &ChatController::onErrorChunk);
    connect(m_runner, &CliRunner::finished,      this, &ChatController::onRunnerFinished);
    connect(m_runner, &CliRunner::toolCallReady, this, &ChatController::onToolCallReady);
    // New connection for simple command results
    connect(m_runner, &CliRunner::simpleCommandFinished, this, &ChatController::onSimpleCommandFinished);

    // Connect ToolExecutor results back to the controller for the autonomous loop
    connect(m_toolExecutor, &ToolExecutor::toolFinished, this, &ChatController::onToolResultReceived);
}

void ChatController::sendMessage(const QString& text, const QList<Attachment>& attachments) {
    if (text.trimmed().isEmpty() && attachments.isEmpty()) return;

    auto* session = m_sessions->currentSession();
    if (!session) {
        session = m_sessions->createSession(m_config->activeProfile(), "default");
        m_sessions->setCurrentSession(session);
    }

    // Build user message
    Message userMsg;
    userMsg.id = QUuid::createUuid();
    userMsg.role = Message::Role::User;
    userMsg.contentBlocks.append(CodeBlock{text, BlockType::Text});
    userMsg.contentTypes.append(Message::ContentType::Text);
    userMsg.attachments = attachments;
    userMsg.timestamp = QDateTime::currentDateTimeUtc();
    userMsg.tokenCount = TokenCounter::estimate(text);

    // Pre-send scripting hooks
    if (m_scripts) m_scripts->runHook("pre_send", {{"text", text}});

    session->appendMessage(userMsg);
    if (!session->save())
        qWarning() << "ChatController: failed to save session after user message";
    emit userMessageReady(userMsg);

    // ── Process attachments ───────────────────────────────────────────────
    // Text/code files → embed content as fenced code blocks in the prompt.
    // Image files      → collect paths; CliRunner/CliProfile handle them via
    //                    imageArguments() (e.g. --image for claude CLI, or
    //                    base64 in JSON for OpenAI-compatible endpoints).
    QString enrichedPrompt = text;
    QStringList imagePaths;

    for (const Attachment& att : attachments) {
        const QFileInfo fi(att.filePath);
        if (!fi.exists()) {
            qWarning() << "ChatController: attachment not found:" << att.filePath;
            continue;
        }
        if (att.type == Attachment::Type::Image) {
            imagePaths << att.filePath;
        } else if (att.type == Attachment::Type::File) {
            QFile f(att.filePath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QString content = QString::fromUtf8(f.readAll());
                enrichedPrompt += "\n\n```" + fi.suffix() + "\n"
                                + "// " + fi.fileName() + "\n"
                                + content + "\n```";
            } else {
                qWarning() << "ChatController: cannot read attachment:" << att.filePath;
            }
        }
        // Audio attachments: not yet sent to model (future: transcription)
    }

    // Start streaming response
    // Pass the full session history so profiles can build multi-turn context.
    // session->messages already includes the user message we just appended.
    m_currentResponse.clear();
    emit generationStarted();
    if (m_runner->isProfileRunning()) {
        emit statusChanged("Agent is thinking...");
    }

    // Update input token stats
    const int inputTokens = TokenCounter::estimate(enrichedPrompt);
    session->updateTokens(inputTokens, 0);

    m_runner->send(enrichedPrompt, m_config->workingFolder(), imagePaths,
                   session->messages);
}

void ChatController::stopGeneration() {
    m_runner->stop();
}

void ChatController::onOutputChunk(const QString& chunk) {
    m_currentResponse += chunk;
    emit tokenReceived(chunk);
}

void ChatController::onRawOutput(const QString& raw) {
    emit consoleOutput(raw);
}

void ChatController::onErrorChunk(const QString& chunk) {
    emit consoleOutput("[stderr] " + chunk);
}

// ── onToolCallReady ───────────────────────────────────────────────────────────
// Called when a complete tool_use block is parsed from the CLI stream.
//
// For Claude CLI profiles the CLI executes the tool itself; this signal
// is used to update the UI and console display.
//
// For future non-Claude profiles with custom tool support, this is where we
// would call m_toolExecutor->execute() and re-prompt with the result.
void ChatController::onToolCallReady(const CodeHex::ToolCall& call) {
    const QString log = formatToolCallLog(call);
    qDebug() << "[Agent] Tool call ready:" << call.name << "with input:" << call.input;
    emit consoleOutput(log);
    emit toolCallStarted(call.name, call.input);

    // Record the tool call in the session history for UI visualization
    QList<CodeBlock> blocks;
    blocks.append(CodeBlock{log, BlockType::ToolCall});
    QList<Message::ContentType> types;
    types.append(Message::ContentType::Code); // Using Code as a generic container

    buildAssistantMessage(blocks, types, log);

    // Trigger execution for custom tool loop
    // This is currently BLOCKING (see ToolExecutor.h)
    // dangerous tools that require approval if enabled
    static const QStringList kDangerous = { "WriteFile", "Write", "RunCommand", "Bash", "Replace", "Sed" };

    if (m_manualApproval && kDangerous.contains(call.name)) {
        qDebug() << "[Agent] Pause for manual approval of:" << call.name;
        emit statusChanged("Waiting for approval...");
        emit toolApprovalRequested(call.name, call.input);
        return;
    }

    emit statusChanged(QString("Executing %1...").arg(call.name));
    m_toolExecutor->execute(call, m_config->workingFolder());
}

void ChatController::setManualApproval(bool enabled) {
    m_manualApproval = enabled;
}

void ChatController::approveToolCall(const CodeHex::ToolCall& call) {
    qDebug() << "[Agent] Resuming approved tool:" << call.name;
    emit statusChanged(QString("Executing %1...").arg(call.name));
    m_toolExecutor->execute(call, m_config->workingFolder());
}

// ── formatToolCallLog ─────────────────────────────────────────────────────────
// Produces a human-readable one-liner like:
//   ⚙ [Tool: Bash] command: "ls -la src/"
//   ⚙ [Tool: Read] path: "src/main.cpp"
QString ChatController::formatToolCallLog(const ToolCall& call) {
    // Show the most useful single-key summary rather than raw JSON
    static const QStringList kPrimaryKeys = {
        "command", "path", "content", "pattern", "root", "file", "n"
    };

    QString summary;
    for (const QString& key : kPrimaryKeys) {
        if (call.input.contains(key)) {
            const QString val = call.input[key].toString();
            if (!val.isEmpty()) {
                summary = key + ": \"" + val.left(120) + (val.length() > 120 ? "…" : "") + "\"";
                break;
            }
        }
    }
    if (summary.isEmpty() && !call.input.isEmpty()) {
        summary = QString::fromUtf8(
            QJsonDocument(call.input).toJson(QJsonDocument::Compact)).left(160);
    }

    return QString("⚙ [Tool: %1] %2").arg(call.name, summary);
}

void ChatController::onRunnerFinished(int exitCode) {
    qDebug() << "[Agent] Profile runner finished with exit code:" << exitCode;

    if (m_currentResponse.trimmed().isEmpty()) {
        emit generationStopped();
        emit statusChanged(""); // Reset status on finish
        return;
    }

    // Parse m_currentResponse for code blocks
    QList<CodeBlock> assistantContentBlocks;
    QList<Message::ContentType> assistantContentTypes;
    QString plainTextResponse; // To accumulate plain text parts

    QRegularExpression bashCodeBlock(R"""(```bash\n(.*?)```)""", QRegularExpression::MultilineOption);
    QRegularExpression pythonCodeBlock(R"""(```python\n(.*?)```)""", QRegularExpression::MultilineOption);
    QRegularExpression luaCodeBlock(R"""(```lua\n(.*?)```)""", QRegularExpression::MultilineOption);

    int lastIndex = 0;
    bool foundBashCommand = false;
    while (lastIndex < m_currentResponse.length()) {
        int nextCodeBlockIndex = -1;
        BlockType nextCodeBlockType = BlockType::Text; // Default to text

        QRegularExpressionMatch bashMatch = bashCodeBlock.match(m_currentResponse, lastIndex);
        QRegularExpressionMatch pythonMatch = pythonCodeBlock.match(m_currentResponse, lastIndex);
        QRegularExpressionMatch luaMatch = luaCodeBlock.match(m_currentResponse, lastIndex);

        // Find the earliest code block
        if (bashMatch.hasMatch() && (nextCodeBlockIndex == -1 || bashMatch.capturedStart() < nextCodeBlockIndex)) {
            nextCodeBlockIndex = bashMatch.capturedStart();
            nextCodeBlockType = BlockType::Bash;
        }
        if (pythonMatch.hasMatch() && (nextCodeBlockIndex == -1 || pythonMatch.capturedStart() < nextCodeBlockIndex)) {
            nextCodeBlockIndex = pythonMatch.capturedStart();
            nextCodeBlockType = BlockType::Python;
        }
        if (luaMatch.hasMatch() && (nextCodeBlockIndex == -1 || luaMatch.capturedStart() < nextCodeBlockIndex)) {
            nextCodeBlockIndex = luaMatch.capturedStart();
            nextCodeBlockType = BlockType::Lua;
        }

        if (nextCodeBlockIndex != -1) {
            // Add preceding text as a Text block
            if (nextCodeBlockIndex > lastIndex) {
                QString textPart = m_currentResponse.mid(lastIndex, nextCodeBlockIndex - lastIndex);
                if (!textPart.trimmed().isEmpty()) {
                    assistantContentBlocks.append(CodeBlock{textPart, BlockType::Text});
                    assistantContentTypes.append(Message::ContentType::Text);
                    plainTextResponse += textPart;
                }
            }

            // Add code block
            QString codeContent;
            int codeBlockEndIndex = -1;
            if (nextCodeBlockType == BlockType::Bash) {
                codeContent = bashMatch.captured(1);
                codeBlockEndIndex = bashMatch.capturedEnd();
                foundBashCommand = true; // Mark that a bash command was found
            } else if (nextCodeBlockType == BlockType::Python) {
                codeContent = pythonMatch.captured(1);
                codeBlockEndIndex = pythonMatch.capturedEnd();
            } else if (nextCodeBlockType == BlockType::Lua) {
                codeContent = luaMatch.captured(1);
                codeBlockEndIndex = luaMatch.capturedEnd();
            }

            if (!codeContent.trimmed().isEmpty()) {
                assistantContentBlocks.append(CodeBlock{codeContent, nextCodeBlockType});
                assistantContentTypes.append(Message::ContentType::Code); // Code type for any code block
                plainTextResponse += codeContent; // Include code in plain text for token counting and display
            }
            lastIndex = codeBlockEndIndex;
        } else {
            // No more code blocks, add remaining text as a Text block
            QString remainingText = m_currentResponse.mid(lastIndex);
            if (!remainingText.trimmed().isEmpty()) {
                assistantContentBlocks.append(CodeBlock{remainingText, BlockType::Text});
                assistantContentTypes.append(Message::ContentType::Text);
                plainTextResponse += remainingText;
            }
            lastIndex = m_currentResponse.length(); // End loop
        }
    }

    // Now, if a tool_use or bash command was found, execute it. Otherwise, build the assistant message.
    QRegularExpression toolUseBlock(R"""(```tool_use\n(.*?)\n```)""", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch toolMatch = toolUseBlock.match(m_currentResponse);

    if (toolMatch.hasMatch()) {
        QString jsonStr = toolMatch.captured(1);
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            ToolCall tc;
            tc.id = obj["id"].toString();
            tc.name = obj["name"].toString();
            tc.input = obj["input"].toObject();

            buildAssistantMessage(assistantContentBlocks, assistantContentTypes, plainTextResponse);
            onToolCallReady(tc);
            return;
        }
    }

    if (foundBashCommand) {
        // Extract the last bash command. We assume only one executable bash command per response for now.
        QString bashCommand;
        for (const auto& block : assistantContentBlocks) {
            if (block.type == BlockType::Bash) {
                bashCommand = block.content;
            }
        }
        if (!bashCommand.isEmpty()) {
            // Add the assistant's message (text leading up to the command)
            buildAssistantMessage(assistantContentBlocks, assistantContentTypes, plainTextResponse);
            // Execute the bash command
            executeBashCommand(bashCommand);
            return; // Don't emit generationStopped yet, it will be emitted by onSimpleCommandFinished
        }
    }

    buildAssistantMessage(assistantContentBlocks, assistantContentTypes, plainTextResponse);
    
    // Update output token stats
    if (m_sessions->currentSession()) {
        const int outputTokens = TokenCounter::estimate(plainTextResponse);
        m_sessions->currentSession()->updateTokens(0, outputTokens);
    }

    emit generationStopped();
    emit statusChanged(""); // Reset status on finish
}

void ChatController::buildAssistantMessage(const QList<CodeBlock>& contentBlocks,
                                           const QList<Message::ContentType>& contentTypes,
                                           const QString& plainText) {
    if (contentBlocks.isEmpty()) return;

    auto* session = m_sessions->currentSession();
    if (!session) return;

    Message assistantMsg;
    assistantMsg.id = QUuid::createUuid();
    assistantMsg.role = Message::Role::Assistant;
    assistantMsg.contentBlocks = contentBlocks;
    assistantMsg.contentTypes = contentTypes;
    assistantMsg.timestamp = QDateTime::currentDateTimeUtc();
    assistantMsg.tokenCount = TokenCounter::estimate(plainText); // Use plainText for token counting

    // Post-receive scripting hooks
    if (m_scripts) m_scripts->runHook("post_receive", {{"text", plainText}});

    session->appendMessage(assistantMsg);
    session->updateTokens(0, assistantMsg.tokenCount);

    // Auto-rename: after the very first exchange (1 user + 1 assistant message),
    // derive a title from the first 6 words of the user's question.
    if (session->messages.size() == 2 && session->title == "default") {
        const QString userText = session->messages.first().textFromContentBlocks();
        const QStringList words = userText.split(' ', Qt::SkipEmptyParts);
        QString autoTitle = words.mid(0, 6).join(' ');
        if (autoTitle.length() > 50)
            autoTitle = autoTitle.left(50);
        if (!autoTitle.isEmpty()) {
            session->title = autoTitle;
            emit sessionRenamed(session->id.toString(QUuid::WithoutBraces), autoTitle);
        }
    }

    if (!session->save())
        qWarning() << "ChatController: failed to save session after assistant message";

    emit responseComplete(assistantMsg);
    m_currentResponse.clear();
}

void ChatController::executeBashCommand(const QString& command) {
    emit consoleOutput(QString("Executing: %1").arg(command));
    emit statusChanged(QString("Executing: %1").arg(command));
    m_runner->runSimpleCommand(command, m_config->workingFolder());
    // The result will be handled by onSimpleCommandFinished
}

void ChatController::onSimpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput) {
    QList<CodeBlock> outputBlocks;
    QList<Message::ContentType> outputContentTypes;
    QString plainTextOutput;

    if (!output.isEmpty()) {
        outputBlocks.append(CodeBlock{output, BlockType::Output});
        outputContentTypes.append(Message::ContentType::Output);
        plainTextOutput += output;
    }
    if (!errorOutput.isEmpty()) {
        outputBlocks.append(CodeBlock{errorOutput, BlockType::Output});
        outputContentTypes.append(Message::ContentType::Output);
        plainTextOutput += "\n" + errorOutput; // Add newline for better formatting
    }

    if (exitCode != 0) {
        QString errorMsg = QString("Command failed with exit code %1.").arg(exitCode);
        if (outputBlocks.isEmpty() && errorOutput.isEmpty()) {
            outputBlocks.append(CodeBlock{errorMsg, BlockType::Output});
            outputContentTypes.append(Message::ContentType::Output);
            plainTextOutput += errorMsg;
        } else {
            // Prepend error message if there's other output
            outputBlocks.prepend(CodeBlock{errorMsg, BlockType::Output});
            outputContentTypes.prepend(Message::ContentType::Output);
            plainTextOutput = errorMsg + "\n" + plainTextOutput;
        }
    }

    buildAssistantMessage(outputBlocks, outputContentTypes, plainTextOutput);

    // Continue the loop: re-prompt the model with the command output
    auto* session = m_sessions->currentSession();
    if (session) {
        emit generationStarted();
        emit statusChanged("Agent is processing result...");
        const int resultTokens = TokenCounter::estimate(plainTextOutput);
        session->updateTokens(resultTokens, 0);
        m_runner->send(plainTextOutput, m_config->workingFolder(), {}, session->messages);
    } else {
        emit generationStopped();
    }
}

void ChatController::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result) {
    Q_UNUSED(toolName)

    auto* session = m_sessions->currentSession();
    if (!session) {
        emit generationStopped();
        return;
    }

    // 1. Log the result to console
    emit consoleOutput(QString("✅ [Result] %1").arg(result.content.left(120)));

    // 2. Add the result as a new message to the session using our helper.
    QList<CodeBlock> blocks;
    blocks.append(CodeBlock{result.content, BlockType::Output});
    QList<Message::ContentType> types;
    types.append(Message::ContentType::Output);

    buildAssistantMessage(blocks, types, result.content);

    // 3. Re-prompt the model automatically.
    // This creates the autonomous loop.
    emit generationStarted();
    
    // Update input tokens for the re-prompt
    const int resultTokens = TokenCounter::estimate(result.content);
    session->updateTokens(resultTokens, 0);

    m_runner->send(result.content, m_config->workingFolder(), {}, session->messages);
}

bool ChatController::isRunning() const {
    return m_runner && m_runner->isRunning();
}

bool ChatController::isProfileRunning() const {
    return m_runner && m_runner->isProfileRunning();
}

}  // namespace CodeHex