#include "PythonEngine.h"
// Qt defines `slots` as a macro which conflicts with Python's PyType_Slot *slots member.
// Undefine it around Python/pybind11 headers.
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")
#include <QDebug>

namespace py = pybind11;

namespace CodeHex {

// Embedded codehex Python module
PYBIND11_EMBEDDED_MODULE(codehex, m) {
    m.doc() = "CodeHex scripting API";
    m.def("log", [](const std::string& msg) {
        qDebug() << "[Python]" << QString::fromStdString(msg);
    });
    m.def("version", []() -> std::string { return "0.1.0"; });
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

void PythonEngine::registerCodeHexModule() {
    // Module already registered via PYBIND11_EMBEDDED_MODULE above
    // Ensure sys.path includes user script directories
    py::module_ sys = py::module_::import("sys");
    // Additional path setup can be done here
}

bool PythonEngine::loadScript(const QString& path) {
    if (!m_initialized) return false;
    try {
        py::gil_scoped_acquire acquire;
        py::eval_file(path.toStdString());
    } catch (const py::error_already_set& e) {
        emit scriptError(path, QString::fromStdString(e.what()));
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
