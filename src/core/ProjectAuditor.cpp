#include "ProjectAuditor.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>

namespace CodeHex {

ProjectAuditor::ProjectAuditor(QObject* parent) : QObject(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ProjectAuditor::runAudit);
}

void ProjectAuditor::setWorkingDirectory(const QString& dir) {
    m_workDir = dir;
}

void ProjectAuditor::start(int intervalMs) {
    m_timer->start(intervalMs);
}

void ProjectAuditor::stop() {
    m_timer->stop();
}

void ProjectAuditor::runAudit() {
    if (m_workDir.isEmpty()) return;
    qDebug() << "[ProjectAuditor] Running background audit...";
    
    checkLargeFiles();
    checkDocumentation();
    checkRulesAdherence();
}

void ProjectAuditor::checkLargeFiles() {
    QDirIterator it(m_workDir, QStringList() << "*.cpp" << "*.h" << "*.hpp", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo info(it.next());
        if (info.size() > 500000) { // Approx 1000+ lines in many cases
             emit auditSuggestion(QString("Large file detected: %1 (%2 KB). Consider refactoring.")
                                 .arg(info.fileName())
                                 .arg(info.size() / 1024));
        }
    }
}

void ProjectAuditor::checkDocumentation() {
    if (m_docsWarned) return; // Emit only once per session
    QDir docsDir(m_workDir + "/docs");
    if (!docsDir.exists()) {
        m_docsWarned = true;
        emit auditSuggestion("No 'docs/' directory found. Good projects deserve good docs!");
    }
}

void ProjectAuditor::checkRulesAdherence() {
    if (m_rulesWarned) return; // Emit only once per session
    QFile rules(m_workDir + "/.agent/rules.md");
    if (!rules.exists()) {
        m_rulesWarned = true;
        emit auditSuggestion("'.agent/rules.md' is missing. Defining rules helps the agent follow your standards.");
    }
}

} // namespace CodeHex
