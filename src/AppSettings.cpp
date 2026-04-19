#include "AppSettings.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QString AppSettings::KEY_PROFILES = "Profiles/Data";
const QString AppSettings::KEY_ACTIVE_PROFILE = "General/ActiveProfileId";
const QString AppSettings::KEY_API_BASE_URL = "General/ApiBaseUrl";
const QString AppSettings::KEY_TTS_API_KEY = "Tts/ApiKey";
const QString AppSettings::KEY_TTS_VOICE = "Tts/Voice";
const QString AppSettings::KEY_TTS_AUTO_PLAY = "Tts/AutoPlay";
const QString AppSettings::KEY_TTS_BASE_URL = "Tts/BaseUrl";

AppSettings::AppSettings(QObject *parent)
    : QObject(parent), m_settings(QSettings::IniFormat, QSettings::UserScope,
                 "FlexiChat", "FlexiChat") {}

void AppSettings::saveProfiles(const QList<SystemPromptProfile> &profiles) {
    QJsonArray arr;
    for (const auto &p : profiles) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["name"] = p.name;
        obj["icon"] = p.icon;
        obj["prompt"] = p.prompt;
        obj["temperature"] = p.temperature;
        obj["maxTokens"] = p.maxTokens;
        arr.append(obj);
    }
    m_settings.setValue(
        KEY_PROFILES, QString(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

QList<SystemPromptProfile> AppSettings::loadProfiles() const {
    QList<SystemPromptProfile> profiles;

    QString data = m_settings.value(KEY_PROFILES).toString();
    if (data.isEmpty()) {
        return profiles;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isArray()) {
        return profiles;
    }

    for (const auto &val : doc.array()) {
        QJsonObject obj = val.toObject();
        SystemPromptProfile p;
        p.id = obj["id"].toString();
        p.name = obj["name"].toString();
        p.icon = obj["icon"].toString();
        p.prompt = obj["prompt"].toString();
        p.temperature = obj["temperature"].toDouble(0.7);
        p.maxTokens = obj["maxTokens"].toInt(2048);
        profiles.append(p);
    }

    return profiles;
}

void AppSettings::saveActiveProfileId(const QString &id) {
    m_settings.setValue(KEY_ACTIVE_PROFILE, id);
}

QString AppSettings::loadActiveProfileId() const {
    return m_settings.value(KEY_ACTIVE_PROFILE).toString();
}

void AppSettings::saveApiBaseUrl(const QString &url) {
    m_settings.setValue(KEY_API_BASE_URL, url);
}

QString AppSettings::loadApiBaseUrl() const {
    return m_settings.value(KEY_API_BASE_URL, "http://localhost:1234").toString();
}

// TTS 設定用関数
void AppSettings::saveTtsApiKey(const QString &key) {
    m_settings.setValue(KEY_TTS_API_KEY, key);
}

QString AppSettings::loadTtsApiKey() const {
    return m_settings.value(KEY_TTS_API_KEY, "").toString();
}

void AppSettings::saveTtsVoice(const QString &voice) {
    m_settings.setValue(KEY_TTS_VOICE, voice);
}

QString AppSettings::loadTtsVoice() const {
    return m_settings.value(KEY_TTS_VOICE, "alloy").toString();
}

void AppSettings::saveTtsAutoPlay(bool enabled) {
    m_settings.setValue(KEY_TTS_AUTO_PLAY, enabled);
}

bool AppSettings::loadTtsAutoPlay() const {
    return m_settings.value(KEY_TTS_AUTO_PLAY, false).toBool();
}

void AppSettings::saveTtsBaseUrl(const QString &url) {
    m_settings.setValue(KEY_TTS_BASE_URL, url);
}

QString AppSettings::loadTtsBaseUrl() const {
    return m_settings.value(KEY_TTS_BASE_URL, "http://localhost:8880").toString();
}
