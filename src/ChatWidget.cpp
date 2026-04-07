#include "ChatWidget.h"
#include "LMStudioClient.h"
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>

/**
 * @brief コンストラクタ
 * UIとクライアントのセットアップを行う
 */
ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent),
      m_chatDisplay(nullptr),
      m_inputField(nullptr),
      m_sendButton(nullptr),
      m_layout(nullptr),
      m_client(nullptr)
{
    setupUI();

    // LM Studioクライアントの初期化
    m_client = new LMStudioClient(this);
    connect(m_client, &LMStudioClient::replyReceived, this, &ChatWidget::onReplyReceived);
    connect(m_client, &LMStudioClient::errorOccurred, this, &ChatWidget::onErrorOccurred);
}

/**
 * @brief デストラクタ
 */
ChatWidget::~ChatWidget() = default;

/**
 * @brief UIのセットアップ
 */
void ChatWidget::setupUI()
{
    // メインレイアウト
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(8);
    m_layout->setContentsMargins(10, 10, 10, 10);

    // チャット表示エリア
    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setPlaceholderText("ここにメッセージが表示されます...");
    m_layout->addWidget(m_chatDisplay);

    // 入力エリアと送信ボタン
    auto *inputLayout = new QHBoxLayout();

    m_inputField = new QLineEdit(this);
    m_inputField->setPlaceholderText("メッセージを入力...");
    m_inputField->returnPressed(); // Enterキーで送信

    m_sendButton = new QPushButton("送信", this);

    inputLayout->addWidget(m_inputField, 1);
    inputLayout->addWidget(m_sendButton);

    m_layout->addLayout(inputLayout);

    // シグナルとスロットの接続
    connect(m_sendButton, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(m_inputField, &QLineEdit::returnPressed, this, &ChatWidget::onSendClicked);
}

/**
 * @brief 送信ボタンがクリックされたときの処理
 */
void ChatWidget::onSendClicked()
{
    QString message = m_inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // ユーザーのメッセージを表示
    appendMessage("user", message);

    // 入力フィールドをクリア
    m_inputField->clear();
    m_inputField->setEnabled(false);
    m_sendButton->setEnabled(false);

    // AIに送信
    m_client->sendRequest(message);
}

/**
 * @brief AIからの応答を受信したときの処理
 */
void ChatWidget::onReplyReceived(const QString &reply)
{
    appendMessage("assistant", reply);

    // 入力を再度有効化
    m_inputField->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_inputField->setFocus();
}

/**
 * @brief エラーが発生したときの処理
 */
void ChatWidget::onErrorOccurred(const QString &error)
{
    appendMessage("error", "エラー: " + error);

    // 入力を再度有効化
    m_inputField->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_inputField->setFocus();
}

/**
 * @brief メッセージをチャット表示に追加
 * @param role 役割（user/assistant/error）
 * @param message メッセージ内容
 */
void ChatWidget::appendMessage(const QString &role, const QString &message)
{
    QString html;
    if (role == "user") {
        html = QString("<p><b style='color: #1a73e8;'>You:</b> %1</p>").arg(message.toHtmlEscaped());
    } else if (role == "assistant") {
        html = QString("<p><b style='color: #0d7c66;'>AI:</b> %1</p>").arg(message.toHtmlEscaped());
    } else {
        html = QString("<p><b style='color: #d93025;'>%1</b></p>").arg(message.toHtmlEscaped());
    }

    m_chatDisplay->append(html);

    // スクロールバーを最下部に移動
    QScrollBar *scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}