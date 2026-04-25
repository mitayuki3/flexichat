#include "MainLogic.h"
#include "AppSettings.h"
#include "OpenAITTSClient.h"

MainLogic::MainLogic(TtsSettingsData const &data, QObject *parent)
    : QObject{parent}, m_autoPlay(data.autoPlay) {
    m_ttsClient = new OpenAITTSClient(data.outputDir, this);
    m_ttsClient->setBaseUrl(data.baseUrl);
    m_ttsClient->setModel(data.model);
    m_ttsClient->setVoice(data.voice);
    m_ttsClient->setInstructions(data.instructions);
    m_ttsClient->setApiKey(data.apiKey);
    // 完了時ハンドラー
    connect(m_ttsClient, &OpenAITTSClient::synthesizeCompleted, this,
            &MainLogic::onSynthesizeCompleted);

    // エラー通知
    connect(m_ttsClient, &OpenAITTSClient::errorOccurred, this,
            &MainLogic::statusOccured);
    connect(m_ttsClient, &OpenAITTSClient::statusChanged, this,
            &MainLogic::statusOccured);
}

void MainLogic::updateSettings(TtsSettingsData const &data) {
    m_autoPlay = data.autoPlay;
    m_ttsClient->setBaseUrl(data.baseUrl);
    m_ttsClient->setModel(data.model);
    m_ttsClient->setVoice(data.voice);
    m_ttsClient->setInstructions(data.instructions);
    m_ttsClient->setApiKey(data.apiKey);
    m_ttsClient->setOutputDir(data.outputDir);
}

void MainLogic::synthesize(QString const &text) {
    if (text.isEmpty()) {
        return;
    }
    m_ttsClient->synthesize(text);
}

void MainLogic::onReplyReceived(QString const &reply) {
    // 自動再生する
    if (m_autoPlay) {
        synthesize(reply);
    }
}

void MainLogic::onSynthesizeCompleted(const QString &filePath) {
    emit ttsFileCreated(filePath);
}
