#include "TokenCounter.h"
#include <QDebug>

#ifdef slots
#undef slots
#endif
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#define slots Q_SLOTS

namespace py = pybind11;

namespace CodeHex::TokenCounter {

static py::object* g_encoding = nullptr;
static bool g_initialized = false;

void init() {
    if (g_initialized) return;
    try {
        py::gil_scoped_acquire acquire;
        py::module_ tiktoken = py::module_::import("tiktoken");
        g_encoding = new py::object(tiktoken.attr("get_encoding")("cl100k_base"));
        g_initialized = true;
        qInfo() << "TokenCounter: Tiktoken (cl100k_base) initialized.";
    } catch (const py::error_already_set& e) {
        qWarning() << "TokenCounter: Failed to initialize Tiktoken:" << e.what();
    }
}

int count(const QString& text) {
    if (text.isEmpty()) return 0;
    if (!g_initialized || !g_encoding) {
        // Fallback to rough estimate if not initialized
        return qMax(1, text.length() / 4);
    }

    try {
        py::gil_scoped_acquire acquire;
        py::list tokens = g_encoding->attr("encode")(text.toStdString());
        return static_cast<int>(py::len(tokens));
    } catch (const py::error_already_set& e) {
        qWarning() << "TokenCounter: Tiktoken error:" << e.what();
        return qMax(1, text.length() / 4);
    }
}

int countMessages(const QList<Message>& messages) {
    int total = 0;
    for (const auto& msg : messages) {
        // Overhead per message (role + formatting) matches OpenAI's approx
        total += 4; 
        total += count(msg.textFromContentBlocks());
    }
    return total;
}

int estimate(const QString& text) {
    return count(text);
}

}  // namespace CodeHex::TokenCounter
