#include "MathLogicTool.h"
#include <QProcess>
#include <QDir>
#include <QDebug>

namespace CodeHex {

ToolResult MathLogicTool::execute(const QJsonObject& input, const QString& workDir) {
    QString expression = input.value("expression").toString();
    if (expression.isEmpty()) {
        return { {}, "No expression provided.", true };
    }

    // Wrap the expression in a SymPy evaluation script
    QString pythonCode = QString(
        "import sympy\n"
        "from sympy import *\n"
        "x, y, z, t = symbols('x y z t')\n"
        "k, m, n = symbols('k m n', integer=True)\n"
        "f, g, h = symbols('f g h', cls=Function)\n"
        "try:\n"
        "    result = eval(\"%1\")\n"
        "    print(result)\n"
        "except Exception as e:\n"
        "    try:\n"
        "        exec(\"%1\")\n"
        "    except Exception as e2:\n"
        "        print(f\"Error: {e2}\")\n"
    ).arg(expression.replace("\"", "\\\""));

    QProcess process;
    process.setWorkingDirectory(workDir);
    process.start("python3", {"-c", pythonCode});
    
    if (!process.waitForFinished(10000)) {
        process.kill();
        return { {}, "Math computation timed out.", true };
    }

    QString output = process.readAllStandardOutput().trimmed();
    QString error = process.readAllStandardError().trimmed();

    if (output.startsWith("Error:") || (!error.isEmpty() && output.isEmpty())) {
        return { {}, error.isEmpty() ? output : error, true };
    }

    return { {}, output, false };
}

} // namespace CodeHex
