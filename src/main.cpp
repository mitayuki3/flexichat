#include "AppSettings.h"
#include "LMStudioClient.h"
#include "MainWindow.h"
#include "OpenAITTSClient.h"
#include "ProfileManager.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QFont>
#include <QScopedPointer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FlexiChat");
    app.setApplicationVersion("1.0.0");

    // 設定の読み込み
    AppSettings settings;

    // LM Studio クライアントの作成
    QScopedPointer<LMStudioClient> client(new LMStudioClient());
    client->setBaseUrl(settings.loadApiBaseUrl());

    // TTS クライアントの作成
    QScopedPointer<OpenAITTSClient> ttsClient(new OpenAITTSClient());
    ttsClient->setBaseUrl(settings.loadTtsBaseUrl());
    ttsClient->setVoice(settings.loadTtsVoice());
    ttsClient->setModel(settings.loadTtsModel());
    ttsClient->setApiKey(settings.loadTtsApiKey());

    // プロファイルマネージャーの作成
    QScopedPointer<ProfileManager> profileManager(new ProfileManager(&settings));
    profileManager->loadProfiles();

    // 初期プロファイルをクライアントに設定
    client->setProfile(profileManager->getActiveProfile());

    // メインウィンドウの作成と表示
    MainWindow mainWindow(profileManager.data());
    mainWindow.show();

    // ProfileManager のシグナルを MainWindow に接続
    QObject::connect(profileManager.data(), &ProfileManager::activeProfileChanged,
                     &mainWindow, &MainWindow::onProfileChanged);
    QObject::connect(profileManager.data(), &ProfileManager::profileListChanged,
                     &mainWindow, &MainWindow::onProfileListChanged);

    // LMStudioClient のシグナルを MainWindow に接続
    QObject::connect(client.data(), &LMStudioClient::replyReceived, &mainWindow,
                     &MainWindow::onReplyReceived);
    QObject::connect(client.data(), &LMStudioClient::errorOccurred, &mainWindow,
                     &MainWindow::onErrorOccurred);
    QObject::connect(client.data(), &LMStudioClient::requestStarted, &mainWindow,
                     &MainWindow::onApiRequestStarted);
    QObject::connect(client.data(), &LMStudioClient::requestCompleted,
                     &mainWindow, &MainWindow::onApiRequestFinished);

    // MainWindow → LMStudioClient のシグナル仲介
    auto *rawClient = client.data();
    QObject::connect(&mainWindow, &MainWindow::requestSend, client.data(),
                     &LMStudioClient::sendRequest);
    QObject::connect(
        &mainWindow, &MainWindow::profileChangeRequested, &mainWindow,
        [rawClient, profileManager = profileManager.data()](const QString &id) {
            auto *profile = profileManager->getProfileById(id);
            if (profile) {
                rawClient->setProfile(*profile);
            }
        });

    // TTS シグナルの接続（MainWindow → OpenAITTSClient）
    QObject::connect(&mainWindow, &MainWindow::synthesizeRequested, ttsClient.data(),
                     [rawTtsClient = ttsClient.data(), &mainWindow]() {
                         QString text = mainWindow.getPendingTtsText();
                         if (!text.isEmpty()) {
                             rawTtsClient->synthesize(text);
                         }
                     });
    QObject::connect(&mainWindow, &MainWindow::stopTtsRequested, ttsClient.data(),
                     &OpenAITTSClient::stop);

    // TTS → MainWindow のシグナル接続（再生状態の更新）
    QObject::connect(ttsClient.data(), &OpenAITTSClient::playbackStarted, &mainWindow,
                     &MainWindow::syncTtsButtons);
    QObject::connect(ttsClient.data(), &OpenAITTSClient::playbackFinished, &mainWindow,
                     &MainWindow::syncTtsButtons);
    QObject::connect(ttsClient.data(), &OpenAITTSClient::errorOccurred, &mainWindow,
                     &MainWindow::onErrorOccurred);

    // 設定ダイアログの表示（シグナル経由で開く）
    QObject::connect(
        &mainWindow, &MainWindow::openSettingsRequested, &mainWindow,
        [rawClient, profileManager = profileManager.data()]() {
            SettingsDialog dialog(profileManager);

            // 接続テストのシグナル仲介
            QObject::connect(&dialog, &SettingsDialog::connectionTestRequested,
                             rawClient, &LMStudioClient::testConnection);
            QObject::connect(rawClient, &LMStudioClient::connectionTestResult,
                             &dialog, &SettingsDialog::onConnectionTestResult);

            dialog.exec();
        });

    return app.exec();
}
