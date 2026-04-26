#include "AppSettings.h"
#include "LMStudioClient.h"
#include "MainLogic.h"
#include "MainWindow.h"
#include "ProfileManager.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QFont>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QScopedPointer>
#include <QThread>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FlexiChat");
    app.setApplicationVersion("1.0.0");

    qRegisterMetaType<TtsSettingsData>();

    QThread *workerThread = new QThread(&app);

    // 設定の読み込み
    AppSettings *settings = new AppSettings(&app);

    TtsSettingsData initData = TtsSettingsData::fromAppSettings(*settings);
    MainLogic *logic = new MainLogic(initData);
    QObject::connect(settings, &AppSettings::changedTts, logic,
                     &MainLogic::updateSettings);

    // LM Studio クライアントの作成
    LMStudioClient *client = new LMStudioClient(&app);
    client->setBaseUrl(settings->loadApiBaseUrl());

    // オーディオプレイヤーの作成
    QMediaPlayer *audioPlayer = new QMediaPlayer(&app);
    QAudioOutput *audioOutput = new QAudioOutput(audioPlayer);
    audioPlayer->setAudioOutput(audioOutput);

    // オーディオデバイス変更の監視
    QMediaDevices *mediaDevices = new QMediaDevices(&app);
    QObject::connect(mediaDevices, &QMediaDevices::audioOutputsChanged,
                     audioOutput, [audioOutput]() {
        QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
        if (audioOutput->device() != defaultDevice) {
            audioOutput->setDevice(defaultDevice);
        }
    });

    // プロファイルマネージャーの作成
    QScopedPointer<ProfileManager> profileManager(new ProfileManager(settings));
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
    QObject::connect(&mainWindow, &MainWindow::autoplayChanged, settings,
                     &AppSettings::saveTtsAutoPlay);

    // TTS タブのボイス変更を保存（保存経由で MainLogic に反映される）
    QObject::connect(&mainWindow, &MainWindow::voiceChanged, settings,
                     &AppSettings::saveTtsVoice);

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

    // TTS シグナルの接続（MainWindow → MainLogic）
    QObject::connect(&mainWindow, &MainWindow::synthesizeRequested, logic,
                     &MainLogic::synthesize);
    QObject::connect(&mainWindow, &MainWindow::ttsPlayRequested, audioPlayer,
                     &QMediaPlayer::play);

    // ファイルパスを QMediaPlayer の音声ソースに設定するヘルパー
    auto setAudioSource = [audioPlayer](const QString &filePath) {
        audioPlayer->setSource(QUrl::fromLocalFile(filePath));
    };
    // 上記に加えて再生まで行うヘルパー
    auto playAudioSource = [audioPlayer,
                            setAudioSource](const QString &filePath) {
        setAudioSource(filePath);
        audioPlayer->play();
    };

    // 音声ファイル生成完了通知（リストへの追加 + 自動再生）
    QObject::connect(logic, &MainLogic::ttsFileCreated, &mainWindow,
                     &MainWindow::appendTtsOutput);
    QObject::connect(logic, &MainLogic::ttsFileCreated, audioPlayer,
                     playAudioSource);

    // 音声ファイルが選択された／アクティベートされた
    QObject::connect(&mainWindow, &MainWindow::ttsFileSelected, audioPlayer,
                     setAudioSource);
    QObject::connect(&mainWindow, &MainWindow::ttsFileActivated, audioPlayer,
                     playAudioSource);

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

    QObject::connect(workerThread, &QThread::finished, logic,
                     &QObject::deleteLater);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, workerThread,
                     [workerThread]() {
                         workerThread->quit();
        workerThread->wait();
    });
    logic->moveToThread(workerThread);
    workerThread->start();
    mainWindow.show();
    return app.exec();
}
