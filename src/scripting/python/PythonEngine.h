#pragma once
#include "../ScriptEngine.h"

namespace CodeHex {

class PythonEngine : public ScriptEngine {
    Q_OBJECT
public:
    explicit PythonEngine(QObject* parent = nullptr);
    ~PythonEngine() override;

    bool initialize() override;
    bool loadScript(const QString& path) override;
    QVariant callHook(const QString& hookName, const QVariantMap& args) override;
    void shutdown() override;
    QString engineName() const override { return "Python"; }

    void setWorkDir(const QString& dir);

signals:
    void appendToChatRequested(const QString& text);

private:
    void registerCodeHexModule();
    bool m_initialized = false;
};

}  // namespace CodeHex
