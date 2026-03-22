#include "PythonEngine.h"
// Qt defines `slots` as a macro which conflicts with Python's PyType_Slot *slots member.
// Undefine it around Python/pybind11 headers.
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QDirIterator>
#include <QRegularExpression>
#include <QSet>

namespace py = pybind11;

namespace CodeHex {

// Global state for the embedded module (pybind11 modules are static — use a singleton)
namespace {
    QString g_workDir;
    std::function<void(const QString&)> g_appendToChat;
}

// Embedded codehex Python module
PYBIND11_EMBEDDED_MODULE(codehex, m) {
    m.doc() = "CodeHex scripting API";

    // ── Basic ────────────────────────────────────────────────────────────────
    m.def("log", [](const std::string& msg) {
        qDebug() << "[Python]" << QString::fromStdString(msg);
    });
    m.def("version", []() -> std::string { return "1.2.0"; });
    m.def("get_work_dir", []() -> std::string { return g_workDir.toStdString(); });

    // ── File I/O ─────────────────────────────────────────────────────────────
    m.def("read_file", [](const std::string& path) -> std::string {
        QString p = QString::fromStdString(path);
        if (QDir::isRelativePath(p)) p = g_workDir + "/" + p;
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return "";
        return f.readAll().toStdString();
    });

    m.def("write_file", [](const std::string& path, const std::string& content) -> bool {
        QString p = QString::fromStdString(path);
        if (QDir::isRelativePath(p)) p = g_workDir + "/" + p;
        QDir().mkpath(QFileInfo(p).path());
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
        f.write(QByteArray::fromStdString(content));
        return true;
    });

    m.def("list_directory", [](const std::string& path) -> py::list {
        QString p = QString::fromStdString(path);
        if (QDir::isRelativePath(p)) p = g_workDir + "/" + p;
        py::list result;
        QDirIterator it(p, QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
        while (it.hasNext()) result.append(it.next().toStdString());
        return result;
    });

    // ── Process ──────────────────────────────────────────────────────────────
    m.def("run_command", [](const std::string& cmd) -> py::dict {
        QProcess proc;
        proc.setWorkingDirectory(g_workDir);
        proc.start("/bin/sh", {"-c", QString::fromStdString(cmd)});
        proc.waitForFinished(30000);
        py::dict d;
        d["stdout"]    = proc.readAllStandardOutput().toStdString();
        d["stderr"]    = proc.readAllStandardError().toStdString();
        d["exit_code"] = proc.exitCode();
        return d;
    });

    m.def("git_status", []() -> std::string {
        QProcess proc;
        proc.setWorkingDirectory(g_workDir);
        proc.start("git", {"status", "--porcelain"});
        proc.waitForFinished(10000);
        return proc.readAllStandardOutput().toStdString();
    });

    // ── Chat ─────────────────────────────────────────────────────────────────
    m.def("append_to_chat", [](const std::string& text) {
        if (g_appendToChat) g_appendToChat(QString::fromStdString(text));
    });
}

PythonEngine::PythonEngine(QObject* parent) : ScriptEngine(parent) {}

PythonEngine::~PythonEngine() {
    shutdown();
}

bool PythonEngine::initialize() {
    if (m_initialized) return true;
    try {
        // Interpreter is now managed by Application
        registerCodeHexModule();
        m_initialized = true;
    } catch (const std::exception& e) {
        qWarning() << "[Python init error]" << e.what();
        return false;
    }
    return true;
}

void PythonEngine::setWorkDir(const QString& dir) {
    g_workDir = dir;
}

void PythonEngine::registerCodeHexModule() {
    // Module already registered via PYBIND11_EMBEDDED_MODULE above.
    // Wire up the append_to_chat callback and sys.path.
    g_appendToChat = [this](const QString& text) {
        emit appendToChatRequested(text);
    };

    try {
        py::gil_scoped_acquire acquire;
        py::module_ sys = py::module_::import("sys");
        py::list path = sys.attr("path");
        path.append("./scripts");
        qDebug() << "[Python] sys.path initialized with ./scripts";
    } catch (const std::exception& e) {
        qWarning() << "[Python] Failed to initialize sys.path:" << e.what();
    }
}

bool PythonEngine::loadScript(const QString& path) {
    if (!m_initialized) return false;
    try {
        py::gil_scoped_acquire acquire;
        py::eval_file(path.toStdString());
    } catch (const py::error_already_set& e) {
        const QString errorMsg = QString::fromStdString(e.what());
        // ModuleNotFoundError is a configuration issue, not a crash — emit a single
        // readable warning instead of flooding the log via scriptError signal.
        if (errorMsg.contains("ModuleNotFoundError") || errorMsg.contains("No module named")) {
            // Extract the missing module name for a concise message
            static QSet<QString> s_reportedModules;
            QRegularExpression re("No module named '([^']+)'");
            QRegularExpressionMatch m = re.match(errorMsg);
            QString moduleName = m.hasMatch() ? m.captured(1) : "unknown";
            if (!s_reportedModules.contains(moduleName)) {
                s_reportedModules.insert(moduleName);
                qWarning() << "[Python] Missing module:" << moduleName
                           << "— install it or remove the script that imports it. (Reported once.)";
            }
            return false;
        }
        emit scriptError(path, errorMsg);
        return false;
    } catch (const std::exception& e) {
        emit scriptError(path, QString::fromStdString(e.what()));
        return false;
    }
    emit scriptLoaded(path);
    return true;
}

QVariant PythonEngine::callHook(const QString& hookName, const QVariantMap& args) {
    if (!m_initialized) return {};
    try {
        py::gil_scoped_acquire acquire;
        py::object main = py::module_::import("__main__");
        if (!py::hasattr(main, hookName.toStdString().c_str())) return {};

        py::object fn = main.attr(hookName.toStdString().c_str());
        py::dict pyArgs;
        for (auto it = args.begin(); it != args.end(); ++it) {
            pyArgs[it.key().toStdString().c_str()] = it.value().toString().toStdString();
        }
        py::object result = fn(pyArgs);
        if (py::isinstance<py::str>(result)) {
            return QString::fromStdString(result.cast<std::string>());
        }
    } catch (const py::error_already_set& e) {
        qWarning() << "[Python hook error]" << hookName << e.what();
    } catch (const std::exception& e) {
        qWarning() << "[Python exception]" << hookName << e.what();
    }
    return {};
}

void PythonEngine::shutdown() {
    if (m_initialized) {
        // Interpreter is now managed by Application
        m_initialized = false;
    }
}

}  // namespace CodeHex
