#include "MainLogic.h"
#include "AppSettings.h"
#include "OpenAITTSClient.h"

MainLogic::MainLogic(AppSettings const &settings, QObject *parent)
    : QObject{parent}, m_autoPlay(settings.loadTtsAutoPlay()) {
    m_ttsClient = new OpenAITTSClient(settings.loadTtsOutputDir(), this);
    m_ttsClient->setBaseUrl(settings.loadTtsBaseUrl());
    m_ttsClient->setModel(settings.loadTtsModel());
    m_ttsClient->setVoice(settings.loadTtsVoice());
    m_ttsClient->setInstructions(settings.loadTtsInstructions());
    m_ttsClient->setApiKey(settings.loadTtsApiKey());
    // 完了時ハンドラー
    connect(m_ttsClient, &OpenAITTSClient::synthesizeCompleted, this,
            &MainLogic::onSynthesizeCompleted);

    // エラー通知
    connect(m_ttsClient, &OpenAITTSClient::errorOccurred, this,
            &MainLogic::statusOccured);
    connect(m_ttsClient, &OpenAITTSClient::statusChanged, this,
            &MainLogic::statusOccured);
}

void MainLogic::updateSettings(AppSettings const &settings) {
    m_autoPlay = settings.loadTtsAutoPlay();
    m_ttsClient->setBaseUrl(settings.loadTtsBaseUrl());
    m_ttsClient->setModel(settings.loadTtsModel());
    m_ttsClient->setVoice(settings.loadTtsVoice());
    m_ttsClient->setInstructions(settings.loadTtsInstructions());
    m_ttsClient->setApiKey(settings.loadTtsApiKey());
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
    QUrl url = QUrl::fromLocalFile(filePath);
    emit mediaSourceChanged(url);
}
