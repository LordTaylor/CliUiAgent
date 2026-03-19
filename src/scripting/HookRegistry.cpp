#include "HookRegistry.h"

namespace CodeHex {

HookRegistry::HookRegistry(QObject* parent) : QObject(parent) {}

void HookRegistry::registerHook(HookPoint point, const QString& scriptId, HookFn fn) {
    m_hooks.append({point, scriptId, std::move(fn)});
}

void HookRegistry::unregisterScript(const QString& scriptId) {
    m_hooks.erase(
        std::remove_if(m_hooks.begin(), m_hooks.end(),
                       [&](const HookEntry& e) { return e.scriptId == scriptId; }),
        m_hooks.end());
}

QVariant HookRegistry::runHooks(HookPoint point, QVariantMap args) const {
    QVariant result;
    for (const auto& entry : m_hooks) {
        if (entry.point == point) {
            result = entry.fn(args);
            // Pass result as updated args for chaining
            if (result.typeId() == QMetaType::QVariantMap) {
                args = result.toMap();
            }
        }
    }
    return result;
}

}  // namespace CodeHex
