#pragma once
#include <QObject>
#include <QVariantMap>
#include <functional>
#include <QString>
#include <QList>

namespace CodeHex {

class HookRegistry : public QObject {
    Q_OBJECT
public:
    enum class HookPoint {
        // Existing
        PreSend,
        PostReceive,
        MessageTransform,
        // New (F3)
        PreToolCall,    // args: {toolName, input} — return modified args or {} to block
        PostToolCall,   // args: {toolName, input, result, isError} — return modified result
        OnFileWrite,    // args: {path, content} — fires after WriteFile succeeds
        OnBuildResult,  // args: {command, stdout, stderr, exitCode}
    };

    using HookFn = std::function<QVariant(QVariantMap)>;

    struct HookEntry {
        HookPoint point;
        QString scriptId;
        HookFn fn;
    };

    explicit HookRegistry(QObject* parent = nullptr);

    void registerHook(HookPoint point, const QString& scriptId, HookFn fn);
    void unregisterScript(const QString& scriptId);
    QVariant runHooks(HookPoint point, QVariantMap args) const;

private:
    QList<HookEntry> m_hooks;
};

}  // namespace CodeHex
