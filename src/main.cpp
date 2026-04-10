#include "AppSettings.h"
#include "LMStudioClient.h"
#include "MainWindow.h"
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

    // LM Studioクライアントの作成
    QScopedPointer<LMStudioClient> client(new LMStudioClient());
    client->setBaseUrl(settings.loadApiBaseUrl());

    // プロファイルマネージャーの作成
    QScopedPointer<ProfileManager> profileManager(new ProfileManager(&settings));
    profileManager->loadProfiles();

    // 初期プロファイルをクライアントに設定
    client->setProfile(profileManager->getActiveProfile());

    // メインウィンドウの作成と表示
    MainWindow mainWindow(profileManager.data());
    mainWindow.show();

    // ProfileManagerのシグナルをMainWindowに接続
    QObject::connect(profileManager.data(), &ProfileManager::activeProfileChanged,
                     &mainWindow, &MainWindow::onProfileChanged);
    QObject::connect(profileManager.data(), &ProfileManager::profileListChanged,
                     &mainWindow, &MainWindow::onProfileListChanged);

    // LMStudioClientのシグナルをMainWindowに接続
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
