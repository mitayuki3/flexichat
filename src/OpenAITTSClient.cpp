#include "OpenAITTSClient.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>

// デフォルトの TTS エンドポイント
static const QString DEFAULT_TTS_ENDPOINT = "/v1/audio/speech";

/**
 * @brief コンストラクタ
 */
OpenAITTSClient::OpenAITTSClient(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
    m_player(new QMediaPlayer(this)), m_audioOutput(new QAudioOutput(this)),
    m_currentReply(nullptr), m_baseUrl("http://localhost:8880"), m_apiKey(""),
    m_voice("alloy"), m_model("tts-1"), m_format("mp3"), m_isPlaying(false) {
    m_networkManager->setTransferTimeout(120000); // 120 秒タイムアウト

    connect(m_networkManager, &QNetworkAccessManager::finished, this,
            &OpenAITTSClient::onSynthesizeFinished);

    // マルチメディアシグナル接続
    m_audioOutput->setParent(m_player); // オーナーはプレイヤー
    m_player->setAudioOutput(m_audioOutput);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia) {
            m_isPlaying = false;
                    emit playbackFinished();
                }
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [=](QMediaPlayer::Error error) {
                Q_UNUSED(error);
        m_isPlaying = false;
                emit errorOccurred("再生エラー");
    });
}

/**
 * @brief デストラクタ
 */
OpenAITTSClient::~OpenAITTSClient() {
    if (m_player) {
        m_player->stop();
    }
}

/**
 * @brief テキストを音声に変換して再生
 * @param text 音声化するテキスト
 * @param format 音声フォーマット（mp3 など）
 */
void OpenAITTSClient::synthesize(const QString &text, const QString &format) {
    if (text.isEmpty() || m_isPlaying) {
        return;
    }

    m_format = format.isEmpty() ? "mp3" : format;

    sendSynthesizeRequest(text);
}

void OpenAITTSClient::playLastResponse() {
    if (!m_tempFile || !m_tempFile->exists()) {
        // まだ生成していない
        return;
    }
    // QMediaPlayer で再生
    QUrl fileUrl = QUrl::fromLocalFile(m_tempFile->fileName());
    m_player->setSource(fileUrl);
    m_player->play();
    m_isPlaying = true;
    emit playbackStarted();
}

/**
 * @brief 再生を停止
 */
void OpenAITTSClient::stop() {
    if (m_player) {
        m_player->stop();
    }
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    m_isPlaying = false;

    // 一時ファイルを削除
    if (m_tempFile) {
        m_tempFile->close();
        m_tempFile->remove();
        m_tempFile.reset();
    }
}

/**
 * @brief 再生中でないか
 */
bool OpenAITTSClient::isPlaying() const { return m_isPlaying; }

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
 * @brief 再生リセット
 */
void OpenAITTSClient::reset() { stop(); }

/**
 * @brief TTS リクエスト送信
 */
void OpenAITTSClient::sendSynthesizeRequest(const QString &text) {
    // テキストチェック
    if (text.isEmpty()) {
        emit errorOccurred("音声化するテキストが空です");
        return;
    }

    if (!m_player || !m_audioOutput) {
        emit errorOccurred("マルチメディア環境が初期化されていません");
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

    // 再生開始メッセージ
    emit errorOccurred("再生中...");

    m_currentReply = m_networkManager->post(request, postData);
    m_isPlaying = true;
}

/**
 * @brief TTS レスポンス受信時
 */
void OpenAITTSClient::onSynthesizeFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        m_isPlaying = false;
        QString errorMsg = reply->errorString();

        if (reply->error() == QNetworkReply::TimeoutError) {
            errorMsg = "接続がタイムアウトしました";
        }

        emit errorOccurred("TTS エラー：" + errorMsg);
        emit errorOccurred("body: " + QString::fromUtf8(reply->readAll()));
        reply->deleteLater();
        return;
    }

    // 音声データ受信
    QByteArray audioData = reply->readAll();

    if (audioData.isEmpty()) {
        m_isPlaying = false;
        emit errorOccurred("音声データが受信できません");
        reply->deleteLater();
        return;
    }

    reply->deleteLater();
    m_currentReply = nullptr;

    // 一時ファイルを作成
    m_tempFile = std::make_unique<QTemporaryFile>(
        QDir::tempPath() + "/flexichat_XXXXXX." + m_format);
    if (!m_tempFile->open()) {
        m_isPlaying = false;
        emit errorOccurred("一時ファイルを作成できません");
        return;
    }

    // 音声データをファイルに書き込み
    m_tempFile->write(audioData);
    m_tempFile->flush();
    m_tempFile->close();

    playLastResponse();
}
