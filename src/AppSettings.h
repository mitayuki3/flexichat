#pragma once

#include "SystemPromptProfile.h"
#include <QList>
#include <QObject>
#include <QSettings>

class AppSettings : public QObject {
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);

    // Profile storage
    void saveProfiles(const QList<SystemPromptProfile> &profiles);
    QList<SystemPromptProfile> loadProfiles() const;

    // Active profile tracking
    void saveActiveProfileId(const QString &id);
    QString loadActiveProfileId() const;

    // Global API settings
    void saveApiBaseUrl(const QString &url);
    QString loadApiBaseUrl() const;

private:
    QSettings m_settings;

    static const QString KEY_PROFILES;
    static const QString KEY_ACTIVE_PROFILE;
    static const QString KEY_API_BASE_URL;
};
