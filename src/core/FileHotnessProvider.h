#pragma once
#include <QString>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QDateTime>
#include <QByteArray>
#include <QList>

namespace CodeHex {

/**
 * @brief Analyzes git history to determine which files are "hot" (most frequently modified).
 */
class FileHotnessProvider : public QObject {
    Q_OBJECT
public:
    explicit FileHotnessProvider(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Scans the git log for the given repository path.
     */
    void scan(const QString& repoPath);

    /**
     * @brief Returns a relative "hotness" score (0-100) based on modification count.
     */
    int getHotness(const QString& absoluteFilePath) const;

    /**
     * @brief Returns true if the path is under a git repository.
     */
    bool isGitAvailable() const { return m_gitAvailable; }

private:
    QString m_repoPath;
    QMap<QString, int> m_counts;
    int m_maxCount = 0;
    bool m_gitAvailable = false;
};

} // namespace CodeHex
