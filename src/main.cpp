#include "AppSettings.h"
#include "LMStudioClient.h"
#include "MainLogic.h"
#include "MainWindow.h"
#include "ProfileManager.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QFont>
#include <QMediaPlayer>
#include <QScopedPointer>
#include <QThread>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FlexiChat");
    app.setApplicationVersion("1.0.0");

    QThread *workerThread = new QThread(&app);
    QObject *workerRoot = new QObject(&app);

    // 設定の読み込み
    AppSettings settings;

    MainLogic *logic = new MainLogic(&settings);

    // LM Studio クライアントの作成
    LMStudioClient *client = new LMStudioClient(workerRoot);
    client->setBaseUrl(settings.loadApiBaseUrl());

    // オーディオプレイヤーの作成
    QMediaPlayer *audioPlayer = new QMediaPlayer(&app);
    QAudioOutput *audioOutput = new QAudioOutput(audioPlayer);
    audioPlayer->setAudioOutput(audioOutput);

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

    QObject::connect(client, &LMStudioClient::replyReceived, logic,
                     &MainLogic::onReplyReceived);
    QObject::connect(logic, &MainLogic::statusOccured, &mainWindow,
                     &MainWindow::showStatusMessage);

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
    QObject::connect(&mainWindow, &MainWindow::synthesizeRequested, logic,
                     &MainLogic::synthesize);
    QObject::connect(&mainWindow, &MainWindow::stopTtsRequested, audioPlayer,
                     &QMediaPlayer::stop);
    QObject::connect(&mainWindow, &MainWindow::ttsPlayRequested, audioPlayer,
                     &QMediaPlayer::play);

    QObject::connect(logic, &MainLogic::mediaSourceChanged, audioPlayer,
                     &QMediaPlayer::setSource);
    QObject::connect(audioPlayer, &QMediaPlayer::sourceChanged, audioPlayer,
                     &QMediaPlayer::play);

    // オーディオプレイヤー状態同期
    QObject::connect(audioPlayer, &QMediaPlayer::playingChanged, &mainWindow,
                     &MainWindow::syncTtsButtons);

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

    logic->moveToThread(workerThread);
    workerThread->start();
    mainWindow.show();
    return app.exec();
}
