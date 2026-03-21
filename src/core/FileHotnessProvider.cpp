#include "FileHotnessProvider.h"
#include <QDir>
#include <QDebug>

namespace CodeHex {

void FileHotnessProvider::scan(const QString& repoPath) {
    m_repoPath = repoPath;
    m_counts.clear();
    m_maxCount = 0;
    m_gitAvailable = false;

    // Check if git is available and repoPath is a git repo
    QProcess checkProc;
    checkProc.setWorkingDirectory(repoPath);
    checkProc.start("git", {"rev-parse", "--is-inside-work-tree"});
    if (!checkProc.waitForFinished() || checkProc.exitCode() != 0) {
        qDebug() << "[FileHotnessProvider] Git not found or not a repo at:" << repoPath;
        return;
    }
    m_gitAvailable = true;

    // Run git log to get file modification counts
    // --name-only lists modified files for each commit
    QProcess logProc;
    logProc.setWorkingDirectory(repoPath);
    logProc.start("git", {"log", "--name-only", "--pretty=format:"});
    
    if (!logProc.waitForFinished(30000)) {
        qDebug() << "[FileHotnessProvider] Git log timed out";
        return;
    }

    QByteArray output = logProc.readAllStandardOutput();
    QList<QByteArray> lines = output.split('\n');

    for (const QByteArray& line : lines) {
        QString file = QString::fromUtf8(line.trimmed());
        if (file.isEmpty()) continue;

        // Path is relative to repo root in git log
        m_counts[file]++;
        if (m_counts[file] > m_maxCount) {
            m_maxCount = m_counts[file];
        }
    }

    qDebug() << "[FileHotnessProvider] Scanned" << m_counts.size() << "files. Max hotness:" << m_maxCount;
}

int FileHotnessProvider::getHotness(const QString& absoluteFilePath) const {
    if (!m_gitAvailable || m_maxCount == 0) return 0;

    QString relativePath = QDir(m_repoPath).relativeFilePath(absoluteFilePath);
    if (!m_counts.contains(relativePath)) return 0;

    // Scale to 0-100
    return (m_counts[relativePath] * 100) / m_maxCount;
}

} // namespace CodeHex
