#pragma once
#include <memory>
#include "../ScriptEngine.h"

// Forward declare sol2 state to keep headers clean
namespace sol { class state; }

namespace CodeHex {

class LuaEngine : public ScriptEngine {
    Q_OBJECT
public:
    explicit LuaEngine(QObject* parent = nullptr);
    ~LuaEngine() override;

    bool initialize() override;
    bool loadScript(const QString& path) override;
    QVariant callHook(const QString& hookName, const QVariantMap& args) override;
    void shutdown() override;
    QString engineName() const override { return "Lua"; }

    void setWorkDir(const QString& dir) { m_workDir = dir; }

signals:
    void appendToChatRequested(const QString& text);

private:
    void registerCodeHexAPI();

    std::unique_ptr<sol::state> m_lua;
    QString m_workDir;
};

}  // namespace CodeHex
