#pragma once
#include <QString>
#include <csignal>

namespace CodeHex {

class CrashHandler {
public:
    static void init();
    static void handleSignal(int sig);
    static void simulateCrash();

private:
    static void saveReport(const QString& reason);
    static QString getStackTrace();
};

} // namespace CodeHex
