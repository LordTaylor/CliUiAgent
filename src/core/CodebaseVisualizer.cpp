#include "CodebaseVisualizer.h"
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

namespace CodeHex {

QString CodebaseVisualizer::generateMermaid(const QString& rootPath, bool includeFiles) {
    QString diagram = "graph TD\n";
    QDir root(rootPath);
    
    // 1. Group files by directory
    QMap<QString, QStringList> dirGroups;
    QDirIterator it(rootPath, {"*.h", "*.cpp", "*.hpp", "*.py"}, QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo info(filePath);
        QString relDir = root.relativeFilePath(info.absolutePath());
        if (relDir.isEmpty()) relDir = ".";
        
        // Skip common build/temp folders
        if (relDir.contains("build") || relDir.contains(".git") || relDir.contains("bin")) continue;
        
        dirGroups[relDir].append(info.fileName());
    }

    // 2. Create nodes for directories and files
    int clusterId = 0;
    QSet<QString> processedNodes;
    
    for (auto it = dirGroups.begin(); it != dirGroups.end(); ++it) {
        QString dirName = it.key();
        QString dirNodeId = "dir_" + QString(dirName).replace("/", "_").replace(".", "_").replace("-", "_");
        
        diagram += QString("  subgraph cluster_%1 [\"%2\"]\n")
                    .arg(clusterId++)
                    .arg(dirName);
        
        if (includeFiles) {
            for (const QString& file : it.value()) {
                QString nodeId = QString(file).replace(".", "_").replace("-", "_");
                diagram += QString("    %1[\"%2\"]\n").arg(nodeId).arg(file);
                processedNodes.insert(nodeId);
            }
        } else {
            diagram += QString("    %1[\"%2\"]\n").arg(dirNodeId).arg(dirName);
        }
        diagram += "  end\n";
    }

    // 3. Simple dependency analysis (includes)
    if (includeFiles) {
        QDirIterator it2(rootPath, {"*.h", "*.cpp", "*.hpp"}, QDir::Files, QDirIterator::Subdirectories);
        QRegularExpression includeRegex("#include\\s*[\"<]([^>\"]+)[\">]");

        while (it2.hasNext()) {
            QString filePath = it2.next();
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QFileInfo srcInfo(filePath);
                QString srcNode = srcInfo.fileName().replace(".", "_").replace("-", "_");
                
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    auto match = includeRegex.match(line);
                    if (match.hasMatch()) {
                        QString targetFile = QFileInfo(match.captured(1)).fileName();
                        QString targetNode = targetFile.replace(".", "_").replace("-", "_");
                        
                        // Only draw edge if targetNode exists in our group to avoid clutter
                        if (processedNodes.contains(targetNode) && srcNode != targetNode) {
                            diagram += QString("  %1 --> %2\n").arg(srcNode).arg(targetNode);
                        }
                    }
                }
            }
        }
    }

    return diagram;
}

} // namespace CodeHex
