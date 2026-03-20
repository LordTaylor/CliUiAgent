#pragma once
#include <QObject>
#include <QTimer>
#include <QStringList>

namespace CodeHex {

/**
 * @brief Background service that proactively scans the project for health/best practices.
 */
class ProjectAuditor : public QObject {
    Q_OBJECT
public:
    explicit ProjectAuditor(QObject* parent = nullptr);

    void setWorkingDirectory(const QString& dir);
    void start(int intervalMs = 60000); // Default: 1 minute
    void stop();

signals:
    /**
     * @brief Emitted when a potential improvement or issue is found.
     */
    void auditSuggestion(const QString& message);

private slots:
    void runAudit();

private:
    void checkLargeFiles();
    void checkDocumentation();
    void checkRulesAdherence();

    QString m_workDir;
    QTimer* m_timer;
};

} // namespace CodeHex
