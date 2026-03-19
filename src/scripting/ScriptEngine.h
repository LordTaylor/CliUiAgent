#pragma once
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace CodeHex {

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    explicit ScriptEngine(QObject* parent = nullptr) : QObject(parent) {}

    virtual bool initialize() = 0;
    virtual bool loadScript(const QString& path) = 0;
    virtual QVariant callHook(const QString& hookName, const QVariantMap& args) = 0;
    virtual void shutdown() = 0;
    virtual QString engineName() const = 0;

signals:
    void scriptLoaded(const QString& path);
    void scriptError(const QString& path, const QString& error);
};

}  // namespace CodeHex
