#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QVariantMap>

namespace CodeHex {

class LuaEngine;
class PythonEngine;

class ScriptManager : public QObject {
    Q_OBJECT
public:
    explicit ScriptManager(const QString& luaDir, const QString& pythonDir,
                           QObject* parent = nullptr);
    ~ScriptManager() override;

    bool initialize();
    void loadAll();
    void runHook(const QString& hookName, const QVariantMap& args);

signals:
    void scriptLoaded(const QString& path);
    void scriptError(const QString& path, const QString& error);

private slots:
    void onDirectoryChanged(const QString& path);

private:
    void loadDirectory(const QString& dir, const QString& ext);

    QString m_luaDir;
    QString m_pythonDir;
    LuaEngine* m_lua = nullptr;
    PythonEngine* m_python = nullptr;
    QFileSystemWatcher m_watcher;
};

}  // namespace CodeHex
