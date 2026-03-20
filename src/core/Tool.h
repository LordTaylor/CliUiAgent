#pragma once
#include <QString>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include "../data/ToolCall.h"

namespace CodeHex {

/**
 * @brief Base interface for all agentic tools.
 */
class Tool {
public:
    virtual ~Tool() = default;

    /**
     * @brief Unique name of the tool (e.g., "read_file", "bash").
     */
    virtual QString name() const = 0;

    /**
     * @brief JSON schema for the tool's parameters.
     */
    virtual QJsonObject parameters() const = 0;

    /**
     * @brief Human-readable description of what the tool does.
     */
    virtual QString description() const = 0;

    /**
     * @brief Executes the tool with the given input.
     * @param input JSON object containing tool arguments.
     * @param workDir Absolute path to the current working directory.
     * @return ToolResult containing the output or error.
     */
    virtual ToolResult execute(const QJsonObject& input, const QString& workDir) = 0;

    /**
     * @brief Aborts the current tool execution if supported.
     */
    virtual void abort() {}
};

} // namespace CodeHex
