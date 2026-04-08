#include "LMStudioClient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QMessageBox>

// デフォルトのLM Studio APIエンドポイント
static const QString DEFAULT_BASE_URL = "http://localhost:1234";
static const QString API_ENDPOINT = "/v1/chat/completions";

/**
 * @brief コンストラクタ
 * ネットワークマネージャーの初期化を行う
 */
LMStudioClient::LMStudioClient(QObject *parent)
    : QObject(parent),
      m_networkManager(nullptr)
{
    m_baseUrl = DEFAULT_BASE_URL;

    // ネットワークマネージャーの作成
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LMStudioClient::onReplyFinished);

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
void LMStudioClient::sendRequest(const QString &message)
{
    // ユーザーメッセージをチャット履歴に追加
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = message;
    m_chatHistory.append(userMessage);

    // リクエストボディの作成
    QJsonObject body;
    body["messages"] = m_chatHistory;
    body["model"] = ""; // LM Studioでロードされているモデルを使用
    body["temperature"] = 0.7;
    body["max_tokens"] = 2048;
    body["stream"] = false; // ストリーミングなし

    // リクエストの作成
    QUrl url(m_baseUrl + API_ENDPOINT);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // ドキュメントのシリアライズ
    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    // POSTリクエストの送信
    m_networkManager->post(request, data);

    // リクエスト開始シグナルを発行
    emit requestStarted();
}

/**
 * @brief レスポンスの受信完了時の処理
 * @param reply ネットワーク返信
 */
void LMStudioClient::onReplyFinished(QNetworkReply *reply)
{
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
void LMStudioClient::handleNetworkError(QNetworkReply *reply)
{
    QString errorMsg;

    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
        errorMsg = "LM Studioに接続できません。LM Studioが起動していて、サーバーが有効になっているか確認してください。\n\nURL: " + m_baseUrl;
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

    emit errorOccurred(errorMsg);

    // リクエスト完了シグナルを発行（エラー時も完了扱い）
    emit requestCompleted();
}