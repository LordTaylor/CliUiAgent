#include "ToolExecutor.h"
#include <functional>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QStringList>

namespace CodeHex {

ToolExecutor::ToolExecutor(QObject* parent) : QObject(parent) {}

// ── Public execute ────────────────────────────────────────────────────────────
ToolResult ToolExecutor::execute(const ToolCall& call, const QString& workDir) {
    emit toolStarted(call.name, call.input);

    ToolResult result;
    const QString& name = call.name;

    if      (name == "ReadFile"      || name == "Read")
        result = execReadFile(call.input, workDir);
    else if (name == "WriteFile"     || name == "Write")
        result = execWriteFile(call.input, workDir);
    else if (name == "ListDirectory" || name == "LS")
        result = execListDirectory(call.input, workDir);
    else if (name == "RunCommand"    || name == "Bash")
        result = execRunCommand(call.input, workDir);
    else if (name == "SearchFiles"   || name == "Glob")
        result = execSearchFiles(call.input, workDir);
    else if (name == "Search"        || name == "Grep")
        result = execSearch(call.input, workDir);
    else if (name == "Replace"       || name == "Sed")
        result = execReplace(call.input, workDir);
    else if (name == "GitStatus")
        result = execGitStatus(workDir);
    else if (name == "GitDiff")
        result = execGitDiff(call.input, workDir);
    else if (name == "GitLog")
        result = execGitLog(call.input, workDir);
    else
        result = errResult(QString("Unknown tool: '%1'").arg(name));

    result.toolUseId = call.id;
    emit toolFinished(call.name, result);
    return result;
}

// ── ReadFile ──────────────────────────────────────────────────────────────────
ToolResult ToolExecutor::execReadFile(const QJsonObject& in, const QString& workDir) {
    const QString path = resolvePath(in["path"].toString(), workDir);
    if (path.isEmpty())
        return errResult("ReadFile: 'path' parameter is required");

    QFile file(path);
    if (!file.exists())
        return errResult(QString("ReadFile: file not found: %1").arg(path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return errResult(QString("ReadFile: cannot open '%1': %2").arg(path, file.errorString()));

    constexpr qint64 kMaxBytes = 100 * 1024;  // 100 KB
    QByteArray data = file.read(kMaxBytes);
    const bool truncated = (file.bytesAvailable() > 0);

    QString content = QString::fromLocal8Bit(data); // Changed from fromUtf8
    if (truncated)
        content += "\n\n[... file truncated at 100 KB ...]";
    return okResult(content);
}

// ── WriteFile ─────────────────────────────────────────────────────────────────
ToolResult ToolExecutor::execWriteFile(const QJsonObject& in, const QString& workDir) {
    const QString path    = resolvePath(in["path"].toString(), workDir);
    const QString content = in["content"].toString();
    if (path.isEmpty())
        return errResult("WriteFile: 'path' parameter is required");

    // Create parent directories if they don't exist
    const QDir dir = QFileInfo(path).dir();
    if (!dir.exists() && !QDir().mkpath(dir.path()))
        return errResult(QString("WriteFile: cannot create directory '%1'").arg(dir.path()));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return errResult(QString("WriteFile: cannot open '%1' for writing: %2")
                         .arg(path, file.errorString()));

    file.write(content.toUtf8());
    return okResult(QString("Written %1 bytes to %2").arg(content.toUtf8().size()).arg(path));
}

// ── ListDirectory ─────────────────────────────────────────────────────────────
ToolResult ToolExecutor::execListDirectory(const QJsonObject& in, const QString& workDir) {
    const QString rawPath = in["path"].toString();
    const QString root    = resolvePath(rawPath.isEmpty() ? "." : rawPath, workDir);
    const int     maxDepth = in.contains("depth") ? in["depth"].toInt() : 3;

    if (!QDir(root).exists())
        return errResult(QString("ListDirectory: path not found: %1").arg(root));

    // Directories to skip
    static const QStringList kSkip = {
        ".git", "node_modules", "build", "target", "__pycache__", ".build",
        ".gradle", ".idea", ".vscode", "dist", "out"
    };

    QStringList lines;
    std::function<void(const QString&, int)> walk = [&](const QString& dir, int depth) {
        if (depth > maxDepth) return;
        const QString indent(static_cast<int>((depth - 1) * 2), ' ');
        const auto entries = QDir(dir).entryInfoList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& fi : entries) {
            if (fi.isDir() && kSkip.contains(fi.fileName())) continue;
            QString details;
            if (!fi.isDir()) {
                const double kb = fi.size() / 1024.0;
                details = QString(" (%1 KB, %2)").arg(kb, 0, 'f', 1).arg(fi.lastModified().toString("yyyy-MM-dd HH:mm"));
            }
            lines << indent + (fi.isDir() ? "[D] " : "    ") + fi.fileName() + details;
            if (fi.isDir()) walk(fi.filePath(), depth + 1);
        }
    };
    walk(root, 1);

    return okResult(lines.isEmpty() ? "(empty directory)" : lines.join('\n'));
}

// ── RunCommand ────────────────────────────────────────────────────────────────
// Runs the command via "bash -c <command>" in workDir.
// Blocks until the process exits or timeout_ms elapses.
// TODO: move to a worker thread for the next sprint.
ToolResult ToolExecutor::execRunCommand(const QJsonObject& in, const QString& workDir) {
    const QString command = in["command"].toString();
    if (command.isEmpty())
        return errResult("RunCommand: 'command' parameter is required");

    const int timeout = in.contains("timeout_ms") ? in["timeout_ms"].toInt() : 30000;

    QProcess proc;
    if (!workDir.isEmpty())
        proc.setWorkingDirectory(workDir);
    proc.start("bash", {"-c", command});

    if (!proc.waitForStarted(5000))
        return errResult(QString("RunCommand: failed to start: %1").arg(proc.errorString()));

    if (!proc.waitForFinished(timeout)) {
        proc.kill();
        return errResult(QString("RunCommand: timed out after %1 ms").arg(timeout));
    }

    const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()); // Changed from fromUtf8
    const QString err = QString::fromLocal8Bit(proc.readAllStandardError());  // Changed from fromUtf8

    QString combined;
    if (!out.isEmpty()) combined += out;
    if (!err.isEmpty()) combined += (combined.isEmpty() ? "" : "\n[stderr]\n") + err;
    if (combined.isEmpty()) combined = "(no output)";

    if (proc.exitCode() != 0)
        return ToolResult{ {}, combined, /*isError=*/true };

    return okResult(combined);
}

// ── SearchFiles ───────────────────────────────────────────────────────────────
ToolResult ToolExecutor::execSearchFiles(const QJsonObject& in, const QString& workDir) {
    const QString pattern = in["pattern"].toString();
    if (pattern.isEmpty())
        return errResult("SearchFiles: 'pattern' parameter is required");

    const QString rawRoot = in.contains("root") ? in["root"].toString() : ".";
    const QString root    = resolvePath(rawRoot, workDir);

    QStringList matches;
    QDirIterator it(root, QStringList{pattern},
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext() && matches.size() < 100)
        matches << QDir(workDir).relativeFilePath(it.next());

    if (matches.isEmpty())
        return okResult("(no files matched the pattern)");
    return okResult(matches.join('\n'));
}

// ── Search ────────────────────────────────────────────────────────────────────
// Grep-like content search.
ToolResult ToolExecutor::execSearch(const QJsonObject& in, const QString& workDir) {
    const QString query = in["query"].toString();
    if (query.isEmpty()) return errResult("Search: 'query' parameter is required");

    const QString root = resolvePath(in.contains("root") ? in["root"].toString() : ".", workDir);
    const bool caseSensitive = in.contains("case_sensitive") ? in["case_sensitive"].toBool() : false;

    QStringList results;
    QDirIterator it(root, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext() && results.size() < 100) {
        const QString filePath = it.next();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            int lineNum = 1;
            while (!stream.atEnd()) {
                const QString line = stream.readLine();
                if (line.contains(query, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                    const QString relPath = QDir(workDir).relativeFilePath(filePath);
                    results << QString("%1:%2: %3").arg(relPath).arg(lineNum).arg(line.trimmed());
                }
                lineNum++;
            }
        }
    }

    if (results.isEmpty()) return okResult("(no matches found)");
    return okResult(results.join('\n'));
}

// ── Replace ───────────────────────────────────────────────────────────────────
// Regex-based text replacement.
ToolResult ToolExecutor::execReplace(const QJsonObject& in, const QString& workDir) {
    const QString path    = resolvePath(in["path"].toString(), workDir);
    const QString pattern = in["pattern"].toString();
    const QString replacement = in["replacement"].toString();

    if (path.isEmpty() || pattern.isEmpty())
        return errResult("Replace: 'path' and 'pattern' parameters are required");

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return errResult(QString("Replace: cannot open '%1' for reading").arg(path));

    QString content = QString::fromLocal8Bit(file.readAll());
    file.close();

    QRegularExpression re(pattern);
    if (!re.isValid())
        return errResult(QString("Replace: invalid regex pattern: %1").arg(re.errorString()));

    const QString newContent = content.replace(re, replacement);
    if (newContent == content)
        return okResult("(no changes made, pattern not found)");

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return errResult(QString("Replace: cannot open '%1' for writing").arg(path));

    file.write(newContent.toUtf8());
    return okResult(QString("Successfully replaced occurrences in %1").arg(path));
}

// ── Git helpers ───────────────────────────────────────────────────────────────
ToolResult ToolExecutor::execGitStatus(const QString& workDir) {
    return execRunCommand(QJsonObject{{"command", "git status --porcelain"}}, workDir);
}

ToolResult ToolExecutor::execGitDiff(const QJsonObject& in, const QString& workDir) {
    const QString file = in["file"].toString();
    const QString cmd  = file.isEmpty() ? "git diff" : "git diff -- " + file;
    return execRunCommand(QJsonObject{{"command", cmd}}, workDir);
}

ToolResult ToolExecutor::execGitLog(const QJsonObject& in, const QString& workDir) {
    const int n   = in.contains("n") ? in["n"].toInt() : 10;
    const QString cmd = QString("git log --oneline -%1").arg(n);
    return execRunCommand(QJsonObject{{"command", cmd}}, workDir);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QString ToolExecutor::resolvePath(const QString& path, const QString& workDir) {
    if (path.isEmpty()) return {};
    if (QDir::isAbsolutePath(path)) return QDir::cleanPath(path);
    return QDir::cleanPath(workDir + '/' + path);
}

ToolResult ToolExecutor::okResult(const QString& content) {
    return ToolResult{ {}, content, false };
}

ToolResult ToolExecutor::errResult(const QString& msg) {
    return ToolResult{ {}, msg, true };
}

}  // namespace CodeHex