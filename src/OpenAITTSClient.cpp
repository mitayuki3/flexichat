#include "OpenAITTSClient.h"
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTime>

// デフォルトの TTS エンドポイント
static const QString DEFAULT_TTS_ENDPOINT = "/v1/audio/speech";

/**
 * @class OpenAITTSClient
 * @brief OpenAI 互換 TTS API クライアント
 * ローカル TTS サーバー（http://localhost:8880）にリクエストを送信し、
 * 返ってきた音声データをファイルとして保存する。
 */
/**
 * @fn OpenAITTSClient::synthesizeCompleted
 * @brief 音声生成完了
 * @param filePath 生成された音声ファイルのパス
 */
/**
 * @fn OpenAITTSClient::errorOccurred
 * @brief エラー発生
 * @param error エラーメッセージ
 */

/**
 * @fn OpenAITTSClient::statusChanged
 * @brief ステータス更新
 * @param status ステータスメッセージ
 */

/**
 * @brief コンストラクタ
 * @param outputDir 音声ファイルの出力先ディレクトリ
 */
OpenAITTSClient::OpenAITTSClient(const QString &outputDir, QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
    m_currentReply(nullptr), m_baseUrl("http://localhost:8880"), m_apiKey(""),
    m_voice("alloy"), m_model("tts-1"), m_format("mp3"),
    m_outputDir(outputDir) {
    m_networkManager->setTransferTimeout(120000); // 120 秒タイムアウト

    connect(m_networkManager, &QNetworkAccessManager::finished, this,
            &OpenAITTSClient::onSynthesizeFinished);
}

/**
 * @brief デストラクタ
 */
OpenAITTSClient::~OpenAITTSClient() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
}

/**
 * @brief テキストを音声に変換してファイルに保存
 * @param text 音声化するテキスト
 */
void OpenAITTSClient::synthesize(const QString &text) {
    if (text.isEmpty()) {
        emit errorOccurred("音声化するテキストが空です");
        return;
    }

    // 以前のリクエストがあれば停止
    stop();

    auto url = QUrl(m_baseUrl + DEFAULT_TTS_ENDPOINT);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "audio/mpeg");

    // API キーがある場合は認証ヘッダー追加
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    // リクエストボディ
    QJsonObject body;
    body["input"] = text;
    if (!m_model.isEmpty()) {
        body["model"] = m_model;
    }
    if (!m_voice.isEmpty()) {
        body["voice"] = m_voice;
    }
    if (!m_instructions.isEmpty()) {
        body["instructions"] = m_instructions;
    }
    if (!m_format.isEmpty()) {
        body["response_format"] = m_format;
    }
    QJsonDocument doc(body);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    // 生成開始メッセージ
    emit statusChanged("音声生成中...");

    m_currentReply = m_networkManager->post(request, postData);
}

/**
 * @brief リクエストを停止
 */
void OpenAITTSClient::stop() {
    if (m_currentReply) {
        // abort() は finished シグナルを同期的に発火することがあり、
        // onSynthesizeFinished 内で m_currentReply が nullptr にリセットされる。
        // そのためローカルに退避してから abort/deleteLater を呼ぶ。
        QNetworkReply *reply = m_currentReply;
        m_currentReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
}

/**
 * @brief ベース URL の設定
 * @param url 例：http://localhost:8880
 */
void OpenAITTSClient::setBaseUrl(const QString &url) { m_baseUrl = url; }

/**
 * @brief API キーの設定
 * @param key API キー
 */
void OpenAITTSClient::setApiKey(const QString &key) { m_apiKey = key; }

/**
 * @brief 音声の種類設定
 * 例：alloy, echo, fable, onyx, nova, shimmer
 * @param voice 音声の種類
 */
void OpenAITTSClient::setVoice(const QString &voice) { m_voice = voice; }

/**
 * @brief モデル設定
 * @param model 例：tts-1
 */
void OpenAITTSClient::setModel(const QString &model) { m_model = model; }

void OpenAITTSClient::setInstructions(const QString &instructions) {
    m_instructions = instructions;
}

/**
 * @brief 音声フォーマット設定（mp3, wav, opus, flac）
 * @param format フォーマット名
 */
void OpenAITTSClient::setFormat(const QString &format) { m_format = format; }

/**
 * @brief 出力先ディレクトリの設定
 * @param dir 出力先ディレクトリ
 */
void OpenAITTSClient::setOutputDir(const QString &dir) { m_outputDir = dir; }

/**
 * @brief リセット
 */
void OpenAITTSClient::reset() { stop(); }

/**
 * @brief ファイルパスを生成
 * @param format ファイルフォーマット
 * @return 生成されたファイルパス
 */
QString OpenAITTSClient::generateFilePath(const QString &format) const {
    // 日付別ディレクトリ
    QString dateDir = QDate::currentDate().toString("yyyyMMdd");
    QDir dir(m_outputDir);
    QString fullDateDir = m_outputDir + "/" + dateDir;
    if (!QDir(fullDateDir).exists()) {
        QDir(m_outputDir).mkpath(dateDir);
    }

    // ファイル名: {時刻}.{format}
    QString timeStr = QTime::currentTime().toString("HHmmss");
    QString fileName = QString("%1.%2").arg(timeStr, format);

    return fullDateDir + "/" + fileName;
}

/**
 * @brief TTS レスポンス受信時
 */
void OpenAITTSClient::onSynthesizeFinished(QNetworkReply *reply) {
    // abort() によるキャンセルは無視
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        if (m_currentReply == reply) {
            m_currentReply = nullptr;
        }
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (m_currentReply == reply) {
            m_currentReply = nullptr;
        }
        QString errorMsg = reply->errorString();

        if (reply->error() == QNetworkReply::TimeoutError) {
            errorMsg = "接続がタイムアウトしました";
        }

        emit errorOccurred("TTS エラー：" + errorMsg);
        qDebug() << "TTS response body:" << QString::fromUtf8(reply->readAll());
        reply->deleteLater();
        return;
    }

    // 音声データ受信
    QByteArray audioData = reply->readAll();

    if (audioData.isEmpty()) {
        if (m_currentReply == reply) {
            m_currentReply = nullptr;
        }
        emit errorOccurred("音声データが受信できません");
        reply->deleteLater();
        return;
    }

    if (m_currentReply == reply) {
        m_currentReply = nullptr;
    }
    reply->deleteLater();

    // ファイルパス生成
    QString filePath = generateFilePath(m_format);
    m_lastFilePath = filePath;

    // ファイルに書き出し
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred("音声ファイルを保存できません: " + filePath);
        return;
    }

    file.write(audioData);
    file.close();

    emit synthesizeCompleted(filePath);
    emit statusChanged("音声生成完了: " + filePath);
}
