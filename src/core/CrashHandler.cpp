#include "CrashHandler.h"
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <execinfo.h>
#include <iostream>

namespace CodeHex {

void CrashHandler::init() {
    signal(SIGSEGV, handleSignal);
    signal(SIGABRT, handleSignal);
    signal(SIGILL,  handleSignal);
}

void CrashHandler::handleSignal(int sig) {
    QString reason;
    switch(sig) {
        case SIGSEGV: reason = "Segmentation Fault (SIGSEGV)"; break;
        case SIGABRT: reason = "Aborted (SIGABRT)"; break;
        case SIGILL:  reason = "Illegal Instruction (SIGILL)"; break;
        default:      reason = "Unknown Signal (" + QString::number(sig) + ")"; break;
    }
    
    saveReport(reason);
    std::cerr << "CodeHex Crashed: " << reason.toStdString() << std::endl;
    exit(sig);
}

void CrashHandler::simulateCrash() {
    int* p = nullptr;
    *p = 42; // SIGSEGV
}

void CrashHandler::saveReport(const QString& reason) {
    QString path = QDir::homePath() + "/.codehex/crashes";
    QDir().mkpath(path);
    
    QString fileName = QString("%1/crash_%2.txt")
        .arg(path)
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "=== CodeHex Crash Report ===\n";
        out << "Time:   " << QDateTime::currentDateTime().toString() << "\n";
        out << "Reason: " << reason << "\n\n";
        out << "Stack Trace:\n" << getStackTrace() << "\n";
        file.close();
    }
}

QString CrashHandler::getStackTrace() {
    void* array[50];
    int size = backtrace(array, 50);
    char** messages = backtrace_symbols(array, size);
    
    QString trace;
    for (int i = 0; i < size; ++i) {
        trace += QString::fromLocal8Bit(messages[i]) + "\n";
    }
    free(messages);
    return trace;
}

} // namespace CodeHex
