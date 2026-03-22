#include "LuaEngine.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QDirIterator>
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

    // ── Basic ────────────────────────────────────────────────────────────────
    codehex.set_function("log", [](const std::string& msg) {
        qDebug() << "[Lua]" << QString::fromStdString(msg);
    });
    codehex.set_function("version", []() -> std::string { return "1.2.0"; });
    codehex.set_function("get_work_dir", [this]() -> std::string {
        return m_workDir.toStdString();
    });

    // ── File I/O ─────────────────────────────────────────────────────────────
    codehex.set_function("read_file", [this](const std::string& path) -> std::string {
        QString p = QString::fromStdString(path);
        if (QDir::isRelativePath(p)) p = m_workDir + "/" + p;
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return "";
        return f.readAll().toStdString();
    });

    codehex.set_function("write_file", [this](const std::string& path,
                                               const std::string& content) -> bool {
        QString p = QString::fromStdString(path);
        if (QDir::isRelativePath(p)) p = m_workDir + "/" + p;
        QDir().mkpath(QFileInfo(p).path());
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
        f.write(QByteArray::fromStdString(content));
        return true;
    });

    codehex.set_function("list_directory", [this](const std::string& path) -> sol::table {
        QString p = QString::fromStdString(path);
        if (QDir::isRelativePath(p)) p = m_workDir + "/" + p;
        sol::table t = m_lua->create_table();
        int idx = 1;
        QDirIterator it(p, QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
        while (it.hasNext()) {
            t[idx++] = it.next().toStdString();
        }
        return t;
    });

    // ── Process ──────────────────────────────────────────────────────────────
    codehex.set_function("run_command", [this](const std::string& cmd) -> sol::table {
        QProcess proc;
        proc.setWorkingDirectory(m_workDir);
        proc.start("/bin/sh", {"-c", QString::fromStdString(cmd)});
        proc.waitForFinished(30000);
        sol::table t = m_lua->create_table();
        t["stdout"]   = proc.readAllStandardOutput().toStdString();
        t["stderr"]   = proc.readAllStandardError().toStdString();
        t["exit_code"] = proc.exitCode();
        return t;
    });

    codehex.set_function("git_status", [this]() -> std::string {
        QProcess proc;
        proc.setWorkingDirectory(m_workDir);
        proc.start("git", {"status", "--porcelain"});
        proc.waitForFinished(10000);
        return proc.readAllStandardOutput().toStdString();
    });

    // ── Chat ─────────────────────────────────────────────────────────────────
    codehex.set_function("append_to_chat", [this](const std::string& text) {
        emit appendToChatRequested(QString::fromStdString(text));
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
