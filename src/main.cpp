#include "AppSettings.h"
#include "LMStudioClient.h"
#include "MainWindow.h"
#include "ProfileManager.h"
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
    MainWindow mainWindow(client.data(), profileManager.data());
    mainWindow.show();

    return app.exec();
}
