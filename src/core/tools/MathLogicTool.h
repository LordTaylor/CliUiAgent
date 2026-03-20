#pragma once
#include "Tool.h"
#include <QJsonObject>
#include <QJsonArray>

namespace CodeHex {

/**
 * @brief Tool for performing advanced mathematical logic and symbolic computation.
 * 
 * Uses Python and SymPy to evaluate complex expressions.
 */
class MathLogicTool : public Tool {
public:
    QString name() const override { return "MathLogic"; }
    QString description() const override { 
        return "Performs symbolic and advanced mathematical computations (calculus, algebra, simplification) using SymPy."; 
    }
    
    QJsonObject parameters() const override {
        QJsonObject root;
        root["type"] = "object";
        QJsonObject props;
        
        QJsonObject expr;
        expr["type"] = "string";
        expr["description"] = "The mathematical expression or Python/SymPy code to evaluate (e.g., 'integrate(sin(x), x)' or 'simplify((x**2 - 1)/(x - 1))').";
        props["expression"] = expr;
        
        root["properties"] = props;
        QJsonArray required;
        required.append("expression");
        root["required"] = required;
        return root;
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override;
};

} // namespace CodeHex
