#include "MainLogic.h"
#include "AppSettings.h"
#include "OpenAITTSClient.h"

MainLogic::MainLogic(AppSettings const *settings, QObject *parent)
    : QObject{parent}, m_settings{settings} {}

void MainLogic::synthesize(QString const &text) {
    if (text.isEmpty()) {
        return;
    }
    AppSettings const &settings = *m_settings;
    OpenAITTSClient *client =
        new OpenAITTSClient(settings.loadTtsOutputDir(), this);

    // 設定適用
    client->setBaseUrl(settings.loadTtsBaseUrl());
    client->setModel(settings.loadTtsModel());
    client->setVoice(settings.loadTtsVoice());
    client->setInstructions(settings.loadTtsInstructions());
    client->setApiKey(settings.loadTtsApiKey());

    // 完了時ハンドラー
    connect(client, &OpenAITTSClient::synthesizeCompleted, this,
            &MainLogic::onSynthesizeCompleted);

    // エラー通知
    connect(client, &OpenAITTSClient::errorOccurred, this,
            &MainLogic::statusOccured);
    connect(client, &OpenAITTSClient::statusChanged, this,
            &MainLogic::statusOccured);

    // 完了時にクリーンアップ
    connect(client, &OpenAITTSClient::synthesizeCompleted, client,
            &QObject::deleteLater);

    client->synthesize(text);
}

void MainLogic::onReplyReceived(QString const &reply) {
    AppSettings const &settings = *m_settings;
    // 自動再生する
    if (settings.loadTtsAutoPlay()) {
        synthesize(reply);
    }
}

void MainLogic::onSynthesizeCompleted(const QString &filePath) {
    QUrl url = QUrl::fromLocalFile(filePath);
    emit mediaSourceChanged(url);
}
