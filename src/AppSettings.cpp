#include "AppSettings.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QString AppSettings::KEY_PROFILES = "Profiles";
const QString AppSettings::KEY_PROFILES_LEGACY = "Profiles/Data";
const QString AppSettings::KEY_ACTIVE_PROFILE = "General/ActiveProfileId";
const QString AppSettings::KEY_API_BASE_URL = "General/ApiBaseUrl";
const QString AppSettings::KEY_TTS_API_KEY = "Tts/ApiKey";
const QString AppSettings::KEY_TTS_VOICE = "Tts/Voice";
const QString AppSettings::KEY_TTS_INSTRUCTIONS = "Tts/Instructions";
const QString AppSettings::KEY_TTS_AUTO_PLAY = "Tts/AutoPlay";
const QString AppSettings::KEY_TTS_BASE_URL = "Tts/BaseUrl";
const QString AppSettings::KEY_TTS_OUTPUT_DIR = "Tts/OutputDir";

TtsSettingsData TtsSettingsData::fromAppSettings(AppSettings const &settings) {
    TtsSettingsData data;
    data.apiKey = settings.loadTtsApiKey();
    data.model = settings.loadTtsModel();
    data.voice = settings.loadTtsVoice();
    data.instructions = settings.loadTtsInstructions();
    data.baseUrl = settings.loadTtsBaseUrl();
    data.outputDir = settings.loadTtsOutputDir();
    data.autoPlay = settings.loadTtsAutoPlay();
    return data;
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent), m_settings(QSettings::IniFormat, QSettings::UserScope,
                 "FlexiChat", "FlexiChat") {}

void AppSettings::saveProfiles(const QList<SystemPromptProfile> &profiles) {
    m_settings.beginWriteArray(KEY_PROFILES, profiles.size());
    for (int i = 0; i < profiles.size(); ++i) {
        const auto &p = profiles.at(i);
        m_settings.setArrayIndex(i);
        m_settings.setValue("id", p.id);
        m_settings.setValue("name", p.name);
        m_settings.setValue("prompt", p.prompt);
        m_settings.setValue("temperature", p.temperature);
        m_settings.setValue("maxTokens", p.maxTokens);
    }
    m_settings.endArray();

    // 旧 JSON 形式のキーが残っていれば削除する
    if (m_settings.contains(KEY_PROFILES_LEGACY)) {
        m_settings.remove(KEY_PROFILES_LEGACY);
    }
}

QList<SystemPromptProfile> AppSettings::loadProfiles() const {
    QList<SystemPromptProfile> profiles;

    int size = m_settings.beginReadArray(KEY_PROFILES);
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        SystemPromptProfile p;
        p.id = m_settings.value("id").toString();
        p.name = m_settings.value("name").toString();
        p.prompt = m_settings.value("prompt").toString();
        p.temperature = m_settings.value("temperature", 0.7).toDouble();
        p.maxTokens = m_settings.value("maxTokens", 2048).toInt();
        profiles.append(p);
    }
    m_settings.endArray();

    if (!profiles.isEmpty()) {
        return profiles;
    }

    // 旧 JSON 形式 (Profiles/Data) からの読み込みフォールバック
    QString legacy = m_settings.value(KEY_PROFILES_LEGACY).toString();
    if (legacy.isEmpty()) {
        return profiles;
    }

    QJsonDocument doc = QJsonDocument::fromJson(legacy.toUtf8());
    if (!doc.isArray()) {
        return profiles;
    }

    for (const auto &val : doc.array()) {
        QJsonObject obj = val.toObject();
        SystemPromptProfile p;
        p.id = obj["id"].toString();
        p.name = obj["name"].toString();
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
    if (key == loadTtsApiKey()) {
        return;
    }
    m_settings.setValue(KEY_TTS_API_KEY, key);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

QString AppSettings::loadTtsApiKey() const {
    return m_settings.value(KEY_TTS_API_KEY, "").toString();
}

void AppSettings::saveTtsModel(const QString &model) {
    if (model == loadTtsModel()) {
        return;
    }
    m_settings.setValue("Tts/Model", model);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

QString AppSettings::loadTtsModel() const {
    return m_settings.value("Tts/Model", "tts-1").toString();
}

void AppSettings::saveTtsVoice(const QString &voice) {
    if (voice == loadTtsVoice()) {
        return;
    }
    m_settings.setValue(KEY_TTS_VOICE, voice);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

QString AppSettings::loadTtsVoice() const {
    return m_settings.value(KEY_TTS_VOICE, "alloy").toString();
}

void AppSettings::saveTtsInstructions(const QString &instructions) {
    if (instructions == loadTtsInstructions()) {
        return;
    }
    m_settings.setValue(KEY_TTS_INSTRUCTIONS, instructions);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

QString AppSettings::loadTtsInstructions() const {
    return m_settings.value(KEY_TTS_INSTRUCTIONS, "").toString();
}

void AppSettings::saveTtsAutoPlay(bool enabled) {
    if (enabled == loadTtsAutoPlay()) {
        return;
    }
    m_settings.setValue(KEY_TTS_AUTO_PLAY, enabled);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

bool AppSettings::loadTtsAutoPlay() const {
    return m_settings.value(KEY_TTS_AUTO_PLAY, false).toBool();
}

void AppSettings::saveTtsBaseUrl(const QString &url) {
    if (url == loadTtsBaseUrl()) {
        return;
    }
    m_settings.setValue(KEY_TTS_BASE_URL, url);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

QString AppSettings::loadTtsBaseUrl() const {
    return m_settings.value(KEY_TTS_BASE_URL, "http://localhost:8880").toString();
}

void AppSettings::saveTtsOutputDir(const QString &dir) {
    if (dir == loadTtsOutputDir()) {
        return;
    }
    m_settings.setValue(KEY_TTS_OUTPUT_DIR, dir);
    emit changedTts(TtsSettingsData::fromAppSettings(*this));
}

QString AppSettings::loadTtsOutputDir() const {
    return m_settings.value(KEY_TTS_OUTPUT_DIR, "./output").toString();
}
