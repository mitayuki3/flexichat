#pragma once

#include "SystemPromptProfile.h"
#include <QList>
#include <QObject>

class AppSettings;

class ProfileManager : public QObject {
    Q_OBJECT

public:
    explicit ProfileManager(AppSettings *settings, QObject *parent = nullptr);

    QList<SystemPromptProfile> getAllProfiles() const;
    SystemPromptProfile getActiveProfile() const;
    QString getActiveProfileId() const;
    SystemPromptProfile *getProfileById(const QString &id);
    const SystemPromptProfile *getProfileById(const QString &id) const;

    void setActiveProfile(const QString &id);
    void addProfile(const SystemPromptProfile &profile);
    void updateProfile(const SystemPromptProfile &profile);
    void deleteProfile(const QString &id);
    void loadProfiles();
    void saveAllProfiles();

    static QList<SystemPromptProfile> builtInDefaults();

signals:
    void activeProfileChanged(const SystemPromptProfile &profile);
    void profileListChanged();

private:
    AppSettings *m_settings;
    QList<SystemPromptProfile> m_profiles;
    QString m_activeProfileId;

    void ensureDefaultProfileExists();
    QString generateUniqueId() const;
};
