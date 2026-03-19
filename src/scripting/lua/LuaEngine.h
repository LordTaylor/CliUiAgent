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

private:
    void registerCodeHexAPI();

    std::unique_ptr<sol::state> m_lua;
};

}  // namespace CodeHex
