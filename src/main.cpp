#include "app/Application.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QString>
#include <QtGlobal>
#include <iostream>

void codehexLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(context);
    
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString typeStr;
    switch (type) {
        case QtDebugMsg:    typeStr = "DBG "; break;
        case QtInfoMsg:     typeStr = "INFO"; break;
        case QtWarningMsg:  typeStr = "WARN"; break;
        case QtCriticalMsg: typeStr = "CRIT"; break;
        case QtFatalMsg:    typeStr = "FTAL"; break;
    }
    
    QString logLine = QString("[%1] [%2] %3\n").arg(timeStr, typeStr, msg);
    
    // Print to console
    std::cerr << logLine.toStdString();
    
    // Append to file
    QString logDir = QDir::homePath() + "/.codehex";
    QDir().mkpath(logDir);
    QFile file(logDir + "/application.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << logLine;
    }
}

#include <QNetworkProxyFactory>

int main(int argc, char* argv[]) {
    // Enable system proxy support (Item 39)
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    qInstallMessageHandler(codehexLogHandler);
    CodeHex::Application app(argc, argv);
    return app.run();
}
