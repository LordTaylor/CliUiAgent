#include "ScriptManager.h"
#include <QDir>
#include "lua/LuaEngine.h"
#include "python/PythonEngine.h"

namespace CodeHex {

ScriptManager::ScriptManager(const QString& luaDir, const QString& pythonDir, QObject* parent)
    : QObject(parent), m_luaDir(luaDir), m_pythonDir(pythonDir) {
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &ScriptManager::onDirectoryChanged);
}

ScriptManager::~ScriptManager() {}

bool ScriptManager::initialize() {
    m_lua = new LuaEngine(this);
    connect(m_lua, &LuaEngine::scriptLoaded, this, &ScriptManager::scriptLoaded);
    connect(m_lua, &LuaEngine::scriptError,  this, &ScriptManager::scriptError);

    m_python = new PythonEngine(this);
    connect(m_python, &PythonEngine::scriptLoaded, this, &ScriptManager::scriptLoaded);
    connect(m_python, &PythonEngine::scriptError,  this, &ScriptManager::scriptError);

    bool ok = m_lua->initialize() && m_python->initialize();

    if (QDir(m_luaDir).exists())    m_watcher.addPath(m_luaDir);
    if (QDir(m_pythonDir).exists()) m_watcher.addPath(m_pythonDir);

    return ok;
}

void ScriptManager::loadAll() {
    loadDirectory(m_luaDir,    "*.lua");
    loadDirectory(m_pythonDir, "*.py");
}

void ScriptManager::loadDirectory(const QString& dir, const QString& ext) {
    QDir d(dir);
    if (!d.exists()) return;
    for (const QString& fname : d.entryList({ext}, QDir::Files)) {
        const QString path = d.filePath(fname);
        if (ext.endsWith("lua") && m_lua)
            m_lua->loadScript(path);
        else if (ext.endsWith("py") && m_python)
            m_python->loadScript(path);
    }
}

void ScriptManager::runHook(const QString& hookName, const QVariantMap& args) {
    if (m_lua)    m_lua->callHook(hookName, args);
    if (m_python) m_python->callHook(hookName, args);
}

void ScriptManager::onDirectoryChanged(const QString& path) {
    // Hot-reload: re-scan changed directory
    if (path == m_luaDir)    loadDirectory(m_luaDir,    "*.lua");
    if (path == m_pythonDir) loadDirectory(m_pythonDir, "*.py");
}

}  // namespace CodeHex
