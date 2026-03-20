#pragma once
#include <QString>
#include <QDir>
#include <QFileInfo>
#include "../data/ToolCall.h"

namespace CodeHex {

class ToolUtils {
public:
    static QString resolvePath(const QString& path, const QString& workDir) {
        if (path.isEmpty()) return {};
        if (QDir::isAbsolutePath(path)) return QDir::cleanPath(path);
        return QDir::cleanPath(workDir + '/' + path);
    }

    static bool isPathSafe(const QString& path, const QString& workDir) {
        if (path.isEmpty()) return true;
        QString absWorkDir = QDir(workDir).absolutePath();
        QFileInfo fi(path);
        QString absPath = fi.isAbsolute() ? fi.absoluteFilePath() : QDir(workDir).absoluteFilePath(path);
        absPath = QDir::cleanPath(absPath);
        return absPath.startsWith(absWorkDir);
    }

    static ToolResult okResult(const QString& content) {
        return ToolResult{ {}, content, false };
    }

    static ToolResult errResult(const QString& msg) {
        return ToolResult{ {}, msg, true };
    }

    static bool isIgnored(const QString& relPath) {
        if (relPath.startsWith("node_modules/") || relPath.contains("/node_modules/")) return true;
        if (relPath.startsWith("build/") || relPath.contains("/build/")) return true;
        if (relPath.startsWith(".git/") || relPath.contains("/.git/")) return true;
        if (relPath.startsWith(".agent/") || relPath.contains("/.agent/")) return true;
        return false;
    }
};

} // namespace CodeHex
