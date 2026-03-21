#include "ModelRouter.h"
#include "AppConfig.h"

namespace CodeHex {

ModelRouter::ModelRouter(AppConfig* config, QObject* parent)
    : QObject(parent), m_config(config) {
    loadDefaults();
}

QString ModelRouter::getProfileIdForRole(AgentRole role) const {
    return m_roleMap.value(role, m_config->activeProviderId());
}

void ModelRouter::setProfileIdForRole(AgentRole role, const QString& profileId) {
    m_roleMap[role] = profileId;
}

void ModelRouter::loadDefaults() {
    // In a real app, this could be loaded from a config file.
    // For now, we use the active provider as a default for all roles.
    QString active = m_config->activeProviderId();
    m_roleMap[AgentRole::Base]     = active;
    m_roleMap[AgentRole::Explorer] = active;
    m_roleMap[AgentRole::Executor] = active;
    m_roleMap[AgentRole::Reviewer] = active;
}

} // namespace CodeHex
