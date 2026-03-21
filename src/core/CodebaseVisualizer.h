#pragma once
#include <QString>
#include <QDir>
#include <QMap>
#include <QStringList>

namespace CodeHex {

/**
 * @brief Analyzes the codebase and generates a Mermaid diagram.
 */
class CodebaseVisualizer {
public:
    /**
     * @brief Generates a top-down Mermaid graph of the project structure.
     * @param rootPath The absolute path to the project root.
     * @param includeFiles If true, shows individual files. If false, only shows directories.
     */
    static QString generateMermaid(const QString& rootPath, bool includeFiles = true);
};

} // namespace CodeHex
