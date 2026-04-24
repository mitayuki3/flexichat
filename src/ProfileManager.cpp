#include "ProfileManager.h"
#include "AppSettings.h"
#include <QUuid>
#include <algorithm>

ProfileManager::ProfileManager(AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

QList<SystemPromptProfile> ProfileManager::getAllProfiles() const {
    return m_profiles;
}

SystemPromptProfile ProfileManager::getActiveProfile() const {
    if (auto *p = getProfileById(m_activeProfileId)) {
        return *p;
    }
    // Fallback: return first profile
    if (!m_profiles.isEmpty()) {
        return m_profiles.first();
    }
    return SystemPromptProfile{};
}

QString ProfileManager::getActiveProfileId() const { return m_activeProfileId; }

SystemPromptProfile *ProfileManager::getProfileById(const QString &id) {
    auto it =
        std::find_if(m_profiles.begin(), m_profiles.end(),
                           [&id](const SystemPromptProfile &p) { return p.id == id; });
    if (it != m_profiles.end()) {
        return &(*it);
    }
    return nullptr;
}

const SystemPromptProfile *
ProfileManager::getProfileById(const QString &id) const {
    auto it =
        std::find_if(m_profiles.begin(), m_profiles.end(),
                           [&id](const SystemPromptProfile &p) { return p.id == id; });
    if (it != m_profiles.end()) {
        return &(*it);
    }
    return nullptr;
}

void ProfileManager::setActiveProfile(const QString &id) {
    if (m_activeProfileId == id) {
        return;
    }

    if (getProfileById(id)) {
        m_activeProfileId = id;
        m_settings->saveActiveProfileId(id);
        emit activeProfileChanged(getActiveProfile());
    }
}

void ProfileManager::addProfile(const SystemPromptProfile &profile) {
    SystemPromptProfile p = profile;
    if (p.id.isEmpty()) {
        p.id = generateUniqueId();
    }
    // Ensure uniqueness
    if (getProfileById(p.id)) {
        p.id = generateUniqueId();
    }
    m_profiles.append(p);
    saveAllProfiles();
    emit profileListChanged();
}

void ProfileManager::updateProfile(const SystemPromptProfile &profile) {
    if (auto *existing = getProfileById(profile.id)) {
        *existing = profile;
        saveAllProfiles();
        emit profileListChanged();

        if (m_activeProfileId == profile.id) {
            emit activeProfileChanged(profile);
        }
    }
}

void ProfileManager::deleteProfile(const QString &id) {
    auto it = std::remove_if(
        m_profiles.begin(), m_profiles.end(),
        [&id](const SystemPromptProfile &p) { return p.id == id; });
    if (it == m_profiles.end()) {
        return;
    }

    m_profiles.erase(it, m_profiles.end());

    // If deleted profile was active, switch to first available
    if (m_activeProfileId == id && !m_profiles.isEmpty()) {
        setActiveProfile(m_profiles.first().id);
    } else if (m_profiles.isEmpty()) {
        // Create a default profile if none remain
        ensureDefaultProfileExists();
        setActiveProfile(m_profiles.first().id);
    }

    saveAllProfiles();
    emit profileListChanged();
}

void ProfileManager::loadProfiles() {
    m_profiles = m_settings->loadProfiles();
    ensureDefaultProfileExists();

    // Load active profile ID
    m_activeProfileId = m_settings->loadActiveProfileId();

    // Validate active profile exists
    if (!getProfileById(m_activeProfileId) && !m_profiles.isEmpty()) {
        m_activeProfileId = m_profiles.first().id;
    }
}

void ProfileManager::saveAllProfiles() { m_settings->saveProfiles(m_profiles); }

QString ProfileManager::getTtsModel() const {
    return m_settings->loadTtsModel();
}

QString ProfileManager::getTtsVoice() const {
    return m_settings->loadTtsVoice();
}

bool ProfileManager::getTtsAutoPlay() const {
    return m_settings->loadTtsAutoPlay();
}

void ProfileManager::saveTtsModel(const QString &model) {
    m_settings->saveTtsModel(model);
}

void ProfileManager::saveTtsVoice(const QString &voice) {
    m_settings->saveTtsVoice(voice);
}

QList<SystemPromptProfile> ProfileManager::builtInDefaults() {
    return {SystemPromptProfile::createDefault("general", "一般", "💬"),
            SystemPromptProfile::createDefault(
                "coding", "コーディング", "💻",
                "You are an expert programming assistant. Provide clear, "
                "well-commented code. "
                "Explain your reasoning step by step."),
            SystemPromptProfile::createDefault(
                "translate", "翻訳", "🌐",
                "あなたは優秀な翻訳アシスタントです。与えられたテキストを正確に、"
                "自然な日本語に翻訳してください。"),
            SystemPromptProfile::createDefault(
                "summarize", "要約", "📋",
                "Provide concise summaries in 2-3 sentences. "
                "Focus on the key points and main ideas.")};
}

void ProfileManager::ensureDefaultProfileExists() {
    if (m_profiles.isEmpty()) {
        auto defaults = builtInDefaults();
        m_profiles = defaults;
        saveAllProfiles();
    }
}

QString ProfileManager::generateUniqueId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
