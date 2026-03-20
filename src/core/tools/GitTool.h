#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include "BashTool.h"

namespace CodeHex {

class GitTool : public Tool {
public:
    enum class Mode { Status, Diff, Log };

    explicit GitTool(Mode mode) : m_mode(mode) {}

    QString name() const override {
        switch (m_mode) {
            case Mode::Status: return "GitStatus";
            case Mode::Diff:   return "GitDiff";
            case Mode::Log:    return "GitLog";
        }
        return "Git";
    }

    QString description() const override {
        switch (m_mode) {
            case Mode::Status: return "Git status --porcelain";
            case Mode::Diff:   return "Git diff [file]";
            case Mode::Log:    return "Git log --oneline";
        }
        return "Git helper";
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        BashTool bash;
        QString cmd;
        switch (m_mode) {
            case Mode::Status: cmd = "git status --porcelain"; break;
            case Mode::Diff: {
                const QString file = input["file"].toString();
                cmd = file.isEmpty() ? "git diff" : "git diff -- " + file;
                break;
            }
            case Mode::Log: {
                const int n = input.contains("n") ? input["n"].toInt() : 10;
                cmd = QString("git log --oneline -%1").arg(n);
                break;
            }
        }
        return bash.execute(QJsonObject{{"command", cmd}}, workDir);
    }

private:
    Mode m_mode;
};

} // namespace CodeHex
