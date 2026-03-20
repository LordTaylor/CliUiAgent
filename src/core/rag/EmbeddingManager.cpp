#include "EmbeddingManager.h"
#include <QDebug>
#include <QDir>
#include <iostream>

#ifdef slots
#undef slots
#endif
#include <pybind11/pybind11.h>
#define slots Q_SLOTS

namespace py = pybind11;

namespace CodeHex {

EmbeddingManager::EmbeddingManager(QObject* parent) : QObject(parent) {
    try {
        initializePython();
    } catch (const py::error_already_set& e) {
        qWarning() << "Python error during EmbeddingManager init:" << e.what();
    }
}

EmbeddingManager::~EmbeddingManager() {
    // scoped_interpreter will take care of shutdown
}

void EmbeddingManager::initializePython() {
    if (m_initialized) return;

    // Add scripts directory to python path
    py::module_ sys = py::module_::import("sys");
    QString scriptPath = QDir::currentPath() + "/scripts";
    sys.attr("path").attr("append")(scriptPath.toStdString());

    try {
        py::module_ backend = py::module_::import("rag_backend");
        m_embeddingFunc = backend.attr("get_embeddings");
        m_initialized = true;
        qInfo() << "EmbeddingManager: Python backend initialized from" << scriptPath;
    } catch (const py::error_already_set& e) {
        qWarning() << "Failed to import rag_backend.py:" << e.what();
    }
}

std::vector<float> EmbeddingManager::getEmbedding(const QString& text) {
    if (!m_initialized) return {};
    auto res = getEmbeddings({text});
    return res.isEmpty() ? std::vector<float>{} : res.first();
}

QList<std::vector<float>> EmbeddingManager::getEmbeddings(const QStringList& texts) {
    if (!m_initialized) return {};

    QList<std::vector<float>> results;
    try {
        py::gil_scoped_acquire acquire;
        py::list pyTexts;
        for (const auto& t : texts) pyTexts.append(t.toStdString());

        py::list pyEmbeds = m_embeddingFunc(pyTexts);
        for (auto item : pyEmbeds) {
            py::list embed = item.cast<py::list>();
            std::vector<float> v;
            for (auto f : embed) v.push_back(f.cast<float>());
            results.append(v);
        }
    } catch (const py::error_already_set& e) {
        qWarning() << "Python error in getEmbeddings:" << e.what();
    }
    return results;
}

} // namespace CodeHex
