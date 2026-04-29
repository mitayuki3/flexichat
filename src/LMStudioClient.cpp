#include "LMStudioClient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

// デフォルトのLM Studio APIエンドポイント
static const QString DEFAULT_BASE_URL = "http://localhost:1234";
static const QString API_ENDPOINT = "/v1/chat/completions";

/**
 * @brief コンストラクタ
 * ネットワークマネージャーの初期化を行う
 */
LMStudioClient::LMStudioClient(QObject *parent)
    : QObject(parent), m_networkManager(nullptr), m_isConnectionTest(false) {
    m_baseUrl = DEFAULT_BASE_URL;

    // ネットワークマネージャーの作成
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this,
            &LMStudioClient::onReplyFinished);

    // チャット履歴の初期化
    m_chatHistory = QJsonArray();
}

/**
 * @brief デストラクタ
 */
LMStudioClient::~LMStudioClient() = default;

/**
 * @brief チャットリクエストの送信
 * @param message ユーザーのメッセージ
 */
void LMStudioClient::sendRequest(const QString &message) {
    // メッセージ配列の構築
    QJsonArray messages;

    // システムプロンプトを最初に追加（存在する場合）
    if (!m_activeProfile.prompt.isEmpty()) {
        QJsonObject sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = m_activeProfile.prompt;
        messages.append(sysMsg);
    }

    // チャット履歴を追加
    for (const auto &historyMsg : m_chatHistory) {
        messages.append(historyMsg.toObject());
    }

    // ユーザーメッセージを追加
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = message;
    messages.append(userMessage);

    // ユーザーメッセージをチャット履歴に追加（次回以降のため）
    m_chatHistory.append(userMessage);

    // リクエストボディの作成
    QJsonObject body;
    body["messages"] = messages;
    if (!m_activeProfile.id.isEmpty()) {
        body["model"] = m_activeProfile.id;
    } else {
        body["model"] = "";
    }
    body["temperature"] = m_activeProfile.temperature;
    body["max_tokens"] = m_activeProfile.maxTokens;
    body["stream"] = false;

    // リクエストの作成
    QUrl url(m_baseUrl + API_ENDPOINT);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // ドキュメントのシリアライズ
    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    m_isConnectionTest = false;

    // POSTリクエストの送信
    m_networkManager->post(request, data);

    // リクエスト開始シグナルを発行
    emit requestStarted();
}

/**
 * @brief レスポンスの受信完了時の処理
 * @param reply ネットワーク返信
 */
void LMStudioClient::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    // HTTPエラーチェック
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }

    // レスポンスの読み込み
    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    // JSONパースエラーチェック
    if (doc.isNull() || !doc.isObject()) {
        emit errorOccurred("Invalid response format");
        return;
    }

    // 応答の抽出
    QJsonObject responseObj = doc.object();
    QJsonArray choices = responseObj.value("choices").toArray();

    if (choices.isEmpty()) {
        emit errorOccurred("No response from AI");
        return;
    }

    QJsonObject firstChoice = choices[0].toObject();
    QJsonObject messageObj = firstChoice.value("message").toObject();
    QString assistantReply = messageObj.value("content").toString();

    // アシスタントの応答をチャット履歴に追加
    QJsonObject assistantMessage;
    assistantMessage["role"] = "assistant";
    assistantMessage["content"] = assistantReply;
    m_chatHistory.append(assistantMessage);

    // 応答シグナルの発行
    emit replyReceived(assistantReply);

    // リクエスト完了シグナルを発行
    emit requestCompleted();
}

/**
 * @brief ネットワークエラーの処理
 * @param reply ネットワーク返信
 */
void LMStudioClient::handleNetworkError(QNetworkReply *reply) {
    QString errorMsg;

    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
        errorMsg = "LM Studioに接続できません。LM "
                   "Studioが起動していて、サーバーが有効になっているか確認してくだ"
                   "さい。\n\nURL: " +
                   m_baseUrl;
        break;
    case QNetworkReply::TimeoutError:
        errorMsg = "接続がタイムアウトしました";
        break;
    case QNetworkReply::ContentNotFoundError:
        errorMsg = "APIエンドポイントが見つかりません";
        break;
    default:
        errorMsg = reply->errorString();
        break;
    }

    if (m_isConnectionTest) {
        emit connectionTestResult(false, errorMsg);
    } else {
        emit errorOccurred(errorMsg);
        emit requestCompleted();
    }
}

/**
 * @brief プロファイルの設定
 * @param profile システムプロンプトプロファイル
 */
void LMStudioClient::setProfile(const SystemPromptProfile &profile) {
    m_activeProfile = profile;
}

/**
 * @brief ベースURLの設定
 */
void LMStudioClient::setBaseUrl(const QString &url) { m_baseUrl = url; }

/**
 * @brief チャット履歴のリセット
 */
void LMStudioClient::resetChatHistory() { m_chatHistory = QJsonArray(); }

/**
 * @brief チャット履歴の置き換え
 */
void LMStudioClient::setChatHistory(const QJsonArray &history) {
    m_chatHistory = history;
}

/**
 * @brief 接続テスト
 */
void LMStudioClient::testConnection() {
    QUrl url(m_baseUrl + "/v1/models");
    QNetworkRequest request(url);

    m_isConnectionTest = true;

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply]() { onConnectionTestFinished(reply); });
}

/**
 * @brief 接続テスト完了時の処理
 */
void LMStudioClient::onConnectionTestFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit connectionTestResult(false, reply->errorString());
    } else {
        emit connectionTestResult(true, "接続に成功しました");
    }
}
