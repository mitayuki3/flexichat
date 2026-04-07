#include "ChatWidget.h"
#include "LMStudioClient.h"
#include "ui_ChatWidget.h"
#include <QMessageBox>
#include <QScrollBar>

/**
 * @brief コンストラクタ
 * UIとクライアントのセットアップを行う
 */
ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::ChatWidget), m_client(nullptr) {
    ui->setupUi(this);

    // LM Studioクライアントの初期化
    m_client = new LMStudioClient(this);
    connect(m_client, &LMStudioClient::replyReceived, this,
            &ChatWidget::onReplyReceived);
    connect(m_client, &LMStudioClient::errorOccurred, this,
            &ChatWidget::onErrorOccurred);

    // ウィジェットのシグナルとスロットを接続
    connect(ui->sendButton, &QPushButton::clicked, this,
            &ChatWidget::onSendClicked);
    connect(ui->inputField, &QLineEdit::returnPressed, this,
            &ChatWidget::onSendClicked);
}

/**
 * @brief デストラクタ
 */
ChatWidget::~ChatWidget() { delete ui; }

/**
 * @brief 送信ボタンがクリックされたときの処理
 */
void ChatWidget::onSendClicked() {
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
void ChatWidget::onReplyReceived(const QString &reply) {
    appendMessage("assistant", reply);

    // 入力を再度有効化
    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
}

/**
 * @brief エラーが発生したときの処理
 */
void ChatWidget::onErrorOccurred(const QString &error) {
    appendMessage("error", "エラー: " + error);

    // 入力を再度有効化
    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
}

/**
 * @brief メッセージをチャット表示に追加
 * @param role 役割（user/assistant/error）
 * @param message メッセージ内容
 */
void ChatWidget::appendMessage(const QString &role, const QString &message) {
    QString html;
    if (role == "user") {
        html = QString("<p><b style='color: #1a73e8;'>You:</b> %1</p>")
                   .arg(message.toHtmlEscaped());
    } else if (role == "assistant") {
        html = QString("<p><b style='color: #0d7c66;'>AI:</b> %1</p>")
                   .arg(message.toHtmlEscaped());
    } else {
        html = QString("<p><b style='color: #d93025;'>%1</b></p>")
                   .arg(message.toHtmlEscaped());
    }

    ui->chatDisplay->append(html);

    // スクロールバーを最下部に移動
    QScrollBar *scrollBar = ui->chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
