#include "AppSettings.h"

const QString AppSettings::KEY_PROFILES = "Profiles/Data";
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
    m_settings.setValue(KEY_PROFILES, QVariant::fromValue(profiles));
}

QList<SystemPromptProfile> AppSettings::loadProfiles() const {
    return m_settings.value(KEY_PROFILES).value<QList<SystemPromptProfile>>();
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
