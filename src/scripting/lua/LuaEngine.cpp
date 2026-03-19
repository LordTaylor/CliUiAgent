#include "LuaEngine.h"
#include <QDebug>
#include <QFile>
#include <sol/sol.hpp>

namespace CodeHex {

LuaEngine::LuaEngine(QObject* parent) : ScriptEngine(parent) {}
LuaEngine::~LuaEngine() { shutdown(); }

bool LuaEngine::initialize() {
    m_lua = std::make_unique<sol::state>();
    m_lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table,
                          sol::lib::math, sol::lib::io, sol::lib::os);
    registerCodeHexAPI();
    return true;
}

void LuaEngine::registerCodeHexAPI() {
    auto codehex = m_lua->create_named_table("codehex");
    codehex.set_function("log", [](const std::string& msg) {
        qDebug() << "[Lua]" << QString::fromStdString(msg);
    });
    codehex.set_function("version", []() -> std::string {
        return "0.1.0";
    });
}

bool LuaEngine::loadScript(const QString& path) {
    if (!m_lua) return false;
    try {
        auto result = m_lua->safe_script_file(path.toStdString());
        if (!result.valid()) {
            sol::error err = result;
            emit scriptError(path, QString::fromStdString(err.what()));
            return false;
        }
    } catch (const std::exception& e) {
        emit scriptError(path, QString::fromStdString(e.what()));
        return false;
    }
    emit scriptLoaded(path);
    return true;
}

QVariant LuaEngine::callHook(const QString& hookName, const QVariantMap& args) {
    if (!m_lua) return {};
    try {
        sol::optional<sol::function> fn = (*m_lua)[hookName.toStdString()];
        if (!fn) return {};
        // Convert QVariantMap to Lua table
        sol::table luaArgs = m_lua->create_table();
        for (auto it = args.begin(); it != args.end(); ++it) {
            luaArgs[it.key().toStdString()] = it.value().toString().toStdString();
        }
        sol::protected_function_result res = (*fn)(luaArgs);
        if (!res.valid()) {
            sol::error err = res;
            qWarning() << "[Lua hook error]" << hookName << err.what();
            return {};
        }
        // Return as string for now
        if (res.get_type() == sol::type::string) {
            return QString::fromStdString(res.get<std::string>());
        }
    } catch (const std::exception& e) {
        qWarning() << "[Lua exception]" << hookName << e.what();
    }
    return {};
}

void LuaEngine::shutdown() {
    m_lua.reset();
}

}  // namespace CodeHex
