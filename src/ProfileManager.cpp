#include "ProfileManager.h"
#include "AppSettings.h"
#include <QUuid>
#include <algorithm>

ProfileManager::ProfileManager(AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

QList<SystemPromptProfile> ProfileManager::getAllProfiles() const {
    QList<SystemPromptProfile> active;
    active.reserve(m_profiles.size());
    for (const auto &p : m_profiles) {
        if (!p.trashed) {
            active.append(p);
        }
    }
    return active;
}

QList<SystemPromptProfile> ProfileManager::getTrashedProfiles() const {
    QList<SystemPromptProfile> trashed;
    for (const auto &p : m_profiles) {
        if (p.trashed) {
            trashed.append(p);
        }
    }
    return trashed;
}

int ProfileManager::getTrashedCount() const {
    return std::count_if(
        m_profiles.begin(), m_profiles.end(),
        [](const SystemPromptProfile &p) { return p.trashed; });
}

SystemPromptProfile ProfileManager::getActiveProfile() const {
    if (auto *p = getProfileById(m_activeProfileId)) {
        return *p;
    }
    // Fallback: return first non-trashed profile
    for (const auto &p : m_profiles) {
        if (!p.trashed) {
            return p;
        }
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

    auto *p = getProfileById(id);
    if (p && !p->trashed) {
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

void ProfileManager::trashProfile(const QString &id) {
    auto *p = getProfileById(id);
    if (!p || p->trashed) {
        return;
    }
    p->trashed = true;

    // ゴミ箱に移したのがアクティブだった場合、別のプロファイルへ切り替え
    if (m_activeProfileId == id) {
        QString nextId;
        for (const auto &prof : m_profiles) {
            if (!prof.trashed) {
                nextId = prof.id;
                break;
            }
        }
        if (nextId.isEmpty()) {
            // 残らない場合はデフォルトを再生成
            ensureDefaultProfileExists();
            for (const auto &prof : m_profiles) {
                if (!prof.trashed) {
                    nextId = prof.id;
                    break;
                }
            }
        }
        m_activeProfileId = nextId;
        m_settings->saveActiveProfileId(nextId);
        saveAllProfiles();
        emit profileListChanged();
        emit activeProfileChanged(getActiveProfile());
        return;
    }

    saveAllProfiles();
    emit profileListChanged();
}

void ProfileManager::restoreProfile(const QString &id) {
    auto *p = getProfileById(id);
    if (!p || !p->trashed) {
        return;
    }
    p->trashed = false;
    saveAllProfiles();
    emit profileListChanged();
}

void ProfileManager::emptyTrash() {
    auto it = std::remove_if(
        m_profiles.begin(), m_profiles.end(),
        [](const SystemPromptProfile &p) { return p.trashed; });
    if (it == m_profiles.end()) {
        return;
    }
    m_profiles.erase(it, m_profiles.end());
    saveAllProfiles();
    emit profileListChanged();
}

void ProfileManager::loadProfiles() {
    m_profiles = m_settings->loadProfiles();
    ensureDefaultProfileExists();

    // Load active profile ID
    m_activeProfileId = m_settings->loadActiveProfileId();

    // Validate: active profile must exist and not be trashed
    auto *active = getProfileById(m_activeProfileId);
    if (!active || active->trashed) {
        m_activeProfileId.clear();
        for (const auto &p : m_profiles) {
            if (!p.trashed) {
                m_activeProfileId = p.id;
                break;
            }
        }
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
    return {SystemPromptProfile::createDefault("general", "一般"),
            SystemPromptProfile::createDefault(
                "coding", "コーディング",
                "You are an expert programming assistant. Provide clear, "
                "well-commented code. "
                "Explain your reasoning step by step."),
            SystemPromptProfile::createDefault(
                "translate", "翻訳",
                "あなたは優秀な翻訳アシスタントです。与えられたテキストを正確に、"
                "自然な日本語に翻訳してください。"),
            SystemPromptProfile::createDefault(
                "summarize", "要約",
                "Provide concise summaries in 2-3 sentences. "
                "Focus on the key points and main ideas.")};
}

void ProfileManager::ensureDefaultProfileExists() {
    bool hasActive = std::any_of(
        m_profiles.begin(), m_profiles.end(),
        [](const SystemPromptProfile &p) { return !p.trashed; });
    if (!hasActive) {
        for (const auto &p : builtInDefaults()) {
            m_profiles.append(p);
        }
        saveAllProfiles();
    }
}

QString ProfileManager::generateUniqueId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
