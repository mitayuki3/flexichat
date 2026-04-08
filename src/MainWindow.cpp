#include "MainWindow.h"
#include "ChatWidget.h"
#include "LMStudioClient.h"
#include "ui_MainWindow.h"
#include <QMessageBox>

/**
 * @brief コンストラクタ
 * UIのセットアップを行う
 */
MainWindow::MainWindow(LMStudioClient *client, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_chatWidget(nullptr) {
    ui->setupUi(this);
    connectSignals();
    setupUI();

    // チャットウィジェットの作成
    m_chatWidget = new ChatWidget(client, this);
    ui->chatContainer->layout()->addWidget(m_chatWidget);

    // LMStudioClientのシグナルをステータスバーに接続
    connect(client, &LMStudioClient::requestStarted, this,
            &MainWindow::onApiRequestStarted);
    connect(client, &LMStudioClient::requestCompleted, this,
            &MainWindow::onApiRequestFinished);
}

/**
 * @brief デストラクタ
 */
MainWindow::~MainWindow() { delete ui; }

/**
 * @brief シグナル接続の設定
 */
void MainWindow::connectSignals() {
    // 終了アクションの接続
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    // バージョン情報アクションの接続
    connect(ui->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "バージョン情報",
                           "FlexiChat v1.0.0\n\nQt + CMake AIチャットアプリ");
    });
}

/**
 * @brief APIリクエスト開始時のステータス更新
 */
void MainWindow::onApiRequestStarted() {
    ui->statusbar->showMessage("応答を生成中...");
}

/**
 * @brief APIリクエスト完了時のステータス更新
 */
void MainWindow::onApiRequestFinished() {
    ui->statusbar->showMessage("準備完了", 3000);
}

/**
 * @brief UIのセットアップ
 */
void MainWindow::setupUI() {
    // APIステータス表示の設定
    ui->statusbar->showMessage("準備完了");
}
