#pragma once
#include "../Tool.h"
#include "ToolUtils.h"
#include "BashTool.h"

namespace CodeHex {

/**
 * @brief Git operations for the agent.
 *
 * Modes (all registered in ToolExecutor):
 *   GitStatus  — git status --porcelain
 *   GitDiff    — git diff [file]
 *   GitLog     — git log --oneline -N
 *   GitAdd     — git add <files...>
 *   GitCommit  — git commit -m <message>
 *   GitCheckout — git checkout <branch>
 *   GitBranches — git branch -a (list all branches)
 *   GitPush    — git push [remote] [branch]
 *   GitStash   — git stash / git stash pop
 */
class GitTool : public Tool {
public:
    enum class Mode {
        Status, Diff, Log,
        Add, Commit, Checkout, Branches, Push, Stash
    };

    explicit GitTool(Mode mode) : m_mode(mode) {}

    QString name() const override {
        switch (m_mode) {
            case Mode::Status:   return "GitStatus";
            case Mode::Diff:     return "GitDiff";
            case Mode::Log:      return "GitLog";
            case Mode::Add:      return "GitAdd";
            case Mode::Commit:   return "GitCommit";
            case Mode::Checkout: return "GitCheckout";
            case Mode::Branches: return "GitBranches";
            case Mode::Push:     return "GitPush";
            case Mode::Stash:    return "GitStash";
        }
        return "Git";
    }

    QString description() const override {
        switch (m_mode) {
            case Mode::Status:   return "Show working tree status (git status --porcelain).";
            case Mode::Diff:     return "Show changes (git diff [file]). Pass 'staged:true' for staged diff.";
            case Mode::Log:      return "Show recent commits (git log --oneline). 'n' sets count (default 10).";
            case Mode::Add:      return "Stage files for commit (git add). Pass 'files' array or '.' for all.";
            case Mode::Commit:   return "Create a commit with given message (git commit -m <message>).";
            case Mode::Checkout: return "Switch branch or restore file (git checkout <branch|file>).";
            case Mode::Branches: return "List all local and remote branches (git branch -a).";
            case Mode::Push:     return "Push commits to remote (git push [remote] [branch]).";
            case Mode::Stash:    return "Stash or pop changes (git stash / git stash pop).";
        }
        return "Git helper";
    }

    QJsonObject parameters() const override {
        switch (m_mode) {
            case Mode::Status:
                return noParams();
            case Mode::Diff:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"file",   QJsonObject{{"type","string"},  {"description","Optional file path"}}},
                    {"staged", QJsonObject{{"type","boolean"}, {"description","Diff staged changes (--cached)"}}}
                }},{"required", QJsonArray{}}};
            case Mode::Log:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"n", QJsonObject{{"type","integer"}, {"description","Number of commits (default 10)"}}}
                }},{"required", QJsonArray{}}};
            case Mode::Add:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"files", QJsonObject{{"type","string"},
                        {"description","Space-separated file paths, or '.' for all changes"}}}
                }},{"required", QJsonArray{"files"}}};
            case Mode::Commit:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"message", QJsonObject{{"type","string"}, {"description","Commit message"}}}
                }},{"required", QJsonArray{"message"}}};
            case Mode::Checkout:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"target", QJsonObject{{"type","string"},
                        {"description","Branch name or file path to checkout"}}}
                }},{"required", QJsonArray{"target"}}};
            case Mode::Branches:
                return noParams();
            case Mode::Push:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"remote", QJsonObject{{"type","string"}, {"description","Remote name (default: origin)"}}},
                    {"branch", QJsonObject{{"type","string"}, {"description","Branch name (default: current)"}}}
                }},{"required", QJsonArray{}}};
            case Mode::Stash:
                return QJsonObject{{"type","object"},{"properties", QJsonObject{
                    {"pop", QJsonObject{{"type","boolean"}, {"description","True to pop the stash"}}}
                }},{"required", QJsonArray{}}};
        }
        return noParams();
    }

    ToolResult execute(const QJsonObject& input, const QString& workDir) override {
        BashTool bash;
        QString cmd;

        switch (m_mode) {
            case Mode::Status:
                cmd = "git status --porcelain";
                break;

            case Mode::Diff: {
                const QString file   = input["file"].toString();
                const bool staged    = input["staged"].toBool(false);
                cmd = staged ? "git diff --cached" : "git diff";
                if (!file.isEmpty()) cmd += " -- " + shellQuote(file);
                break;
            }

            case Mode::Log: {
                const int n = input.contains("n") ? input["n"].toInt() : 10;
                cmd = QString("git log --oneline -%1").arg(n);
                break;
            }

            case Mode::Add: {
                const QString files = input["files"].toString().trimmed();
                if (files.isEmpty())
                    return ToolUtils::errResult("GitAdd: 'files' parameter is required");
                cmd = "git add " + files;
                break;
            }

            case Mode::Commit: {
                const QString msg = input["message"].toString().trimmed();
                if (msg.isEmpty())
                    return ToolUtils::errResult("GitCommit: 'message' parameter is required");
                cmd = "git commit -m " + shellQuote(msg);
                break;
            }

            case Mode::Checkout: {
                const QString target = input["target"].toString().trimmed();
                if (target.isEmpty())
                    return ToolUtils::errResult("GitCheckout: 'target' parameter is required");
                cmd = "git checkout " + shellQuote(target);
                break;
            }

            case Mode::Branches:
                cmd = "git branch -a";
                break;

            case Mode::Push: {
                const QString remote = input["remote"].toString().trimmed();
                const QString branch = input["branch"].toString().trimmed();
                cmd = "git push";
                if (!remote.isEmpty()) cmd += " " + remote;
                if (!branch.isEmpty()) cmd += " " + branch;
                break;
            }

            case Mode::Stash:
                cmd = input["pop"].toBool(false) ? "git stash pop" : "git stash";
                break;
        }

        return bash.execute(QJsonObject{{"command", cmd}}, workDir);
    }

private:
    Mode m_mode;

    static QJsonObject noParams() {
        return QJsonObject{{"type","object"},{"properties",QJsonObject{}},{"required",QJsonArray{}}};
    }

    // Wrap in single quotes, escaping any embedded single quotes
    static QString shellQuote(const QString& s) {
        return "'" + QString(s).replace("'", "'\\''") + "'";
    }
};

} // namespace CodeHex
