#include "MainWindow.h"
#include "LMStudioClient.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QScrollBar>

/**
 * @brief コンストラクタ
 * UIのセットアップを行う
 */
MainWindow::MainWindow(LMStudioClient *client, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_client(client),
      m_model(new QStringListModel(this)) {
    ui->setupUi(this);

    // モデルのセットアップ
    ui->chatDisplay->setModel(m_model);

    connectSignals();
    setupUI();

    // LMStudioClientのシグナルを接続
    connect(m_client, &LMStudioClient::replyReceived, this,
            &MainWindow::onReplyReceived);
    connect(m_client, &LMStudioClient::errorOccurred, this,
            &MainWindow::onErrorOccurred);
    connect(m_client, &LMStudioClient::requestStarted, this,
            &MainWindow::onApiRequestStarted);
    connect(m_client, &LMStudioClient::requestCompleted, this,
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

    // ウィジェットのシグナルとスロットを接続
    connect(ui->sendButton, &QPushButton::clicked, this,
            &MainWindow::onSendClicked);
    connect(ui->inputField, &QLineEdit::returnPressed, this,
            &MainWindow::onSendClicked);
}

/**
 * @brief 送信ボタンがクリックされたときの処理
 */
void MainWindow::onSendClicked() {
    QString message = ui->inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // ユーザーのメッセージを表示
    appendMessage("user", message);

    // 入力フィールドをクリア
    ui->inputField->clear();
    ui->inputField->setEnabled(false);
    ui->sendButton->setEnabled(false);

    // AIに送信
    m_client->sendRequest(message);
}

/**
 * @brief AIからの応答を受信したときの処理
 */
void MainWindow::onReplyReceived(const QString &reply) {
    appendMessage("assistant", reply);

    // 入力を再度有効化
    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
}

/**
 * @brief エラーが発生したときの処理
 */
void MainWindow::onErrorOccurred(const QString &error) {
    appendMessage("error", "エラー: " + error);

    // 入力を再度有効化
    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
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

/**
 * @brief メッセージをチャット表示に追加
 * @param role 役割（user/assistant/error）
 * @param message メッセージ内容
 */
void MainWindow::appendMessage(const QString &role, const QString &message) {
    QString displayMessage;
    if (role == "user") {
        displayMessage = QString("You: %1").arg(message);
    } else if (role == "assistant") {
        displayMessage = QString("AI: %1").arg(message);
    } else {
        displayMessage = message;
    }

    // モデルに行を追加
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setData(m_model->index(row), displayMessage);

    // 最下部までスクロール
    ui->chatDisplay->scrollToBottom();
}
