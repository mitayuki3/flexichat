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

    QObject *workerRoot = new QObject(&app);

    // 設定の読み込み
    AppSettings settings;

    // LM Studio クライアントの作成
    LMStudioClient *client = new LMStudioClient(workerRoot);
    client->setBaseUrl(settings.loadApiBaseUrl());

    // TTS クライアントの作成
    OpenAITTSClient *ttsClient = new OpenAITTSClient(workerRoot);
    ttsClient->setBaseUrl(settings.loadTtsBaseUrl());
    ttsClient->setModel(settings.loadTtsModel());
    ttsClient->setVoice(settings.loadTtsVoice());
    ttsClient->setInstructions(settings.loadTtsInstructions());
    ttsClient->setApiKey(settings.loadTtsApiKey());

    // プロファイルマネージャーの作成
    QScopedPointer<ProfileManager> profileManager(new ProfileManager(&settings));
    profileManager->loadProfiles();

    // 初期プロファイルをクライアントに設定
    client->setProfile(profileManager->getActiveProfile());

    // メインウィンドウ
    MainWindow mainWindow(profileManager.data());

    // ProfileManager のシグナルを MainWindow に接続
    QObject::connect(profileManager.data(), &ProfileManager::activeProfileChanged,
                     &mainWindow, &MainWindow::onProfileChanged);
    QObject::connect(profileManager.data(), &ProfileManager::profileListChanged,
                     &mainWindow, &MainWindow::onProfileListChanged);

    // LMStudioClient のシグナルを MainWindow に接続
    QObject::connect(client, &LMStudioClient::replyReceived, &mainWindow,
                     &MainWindow::onReplyReceived);
    QObject::connect(client, &LMStudioClient::errorOccurred, &mainWindow,
                     &MainWindow::onErrorOccurred);
    QObject::connect(client, &LMStudioClient::requestStarted, &mainWindow,
                     &MainWindow::onApiRequestStarted);
    QObject::connect(client, &LMStudioClient::requestCompleted, &mainWindow,
                     &MainWindow::onApiRequestFinished);

    QObject::connect(client, &LMStudioClient::replyReceived, ttsClient,
                     [ttsClient, &settings](const QString &reply) {
        // 自動再生する
        if (settings.loadTtsAutoPlay()) {
            ttsClient->synthesize(reply);
        }
    });

    // MainWindow のチェックボックス状態変化を保存
    QObject::connect(&mainWindow, &MainWindow::autoplayChanged, &settings,
                     &AppSettings::saveTtsAutoPlay);

    // MainWindow → LMStudioClient のシグナル仲介
    QObject::connect(&mainWindow, &MainWindow::requestSend, client,
                     &LMStudioClient::sendRequest);
    QObject::connect(
        &mainWindow, &MainWindow::profileChangeRequested, &mainWindow,
        [client, profileManager = profileManager.data()](const QString &id) {
            auto *profile = profileManager->getProfileById(id);
            if (profile) {
                client->setProfile(*profile);
            }
        });

    // TTS シグナルの接続（MainWindow → OpenAITTSClient）
    QObject::connect(&mainWindow, &MainWindow::synthesizeRequested, ttsClient,
                     [ttsClient, &mainWindow]() {
        QString text = mainWindow.getPendingTtsText();
        if (!text.isEmpty()) {
            ttsClient->synthesize(text);
        }
    });
    QObject::connect(&mainWindow, &MainWindow::stopTtsRequested, ttsClient,
                     &OpenAITTSClient::stop);
    QObject::connect(&mainWindow, &MainWindow::ttsPlayRequested, ttsClient,
                     &OpenAITTSClient::playLastResponse);

    // TTS → MainWindow のシグナル接続（再生状態の更新）
    QObject::connect(ttsClient, &OpenAITTSClient::playbackStarted, &mainWindow,
                     &MainWindow::syncTtsButtons);
    QObject::connect(ttsClient, &OpenAITTSClient::playbackFinished, &mainWindow,
                     &MainWindow::syncTtsButtons);
    QObject::connect(ttsClient, &OpenAITTSClient::errorOccurred, &mainWindow,
                     &MainWindow::onErrorOccurred);

    // 設定ダイアログの表示（シグナル経由で開く）
    QObject::connect(
        &mainWindow, &MainWindow::openSettingsRequested, &mainWindow,
        [client, profileManager = profileManager.data()]() {
            SettingsDialog dialog(profileManager);

            // 接続テストのシグナル仲介
            QObject::connect(&dialog, &SettingsDialog::connectionTestRequested,
                             client, &LMStudioClient::testConnection);
            QObject::connect(client, &LMStudioClient::connectionTestResult, &dialog,
                             &SettingsDialog::onConnectionTestResult);

            dialog.exec();
        });

    mainWindow.show();
    return app.exec();
}
