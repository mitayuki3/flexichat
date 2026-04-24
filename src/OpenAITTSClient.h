#pragma once

#include <QAudioOutput>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTemporaryFile>
#include <QUrl>
#include <memory>

/**
 * @brief OpenAI 互換 TTS API クライアント
 * ローカル TTS サーバー（http://localhost:8880）にリクエストを送信し、
 * 返ってきた音声データを QMediaPlayer で再生する。
 */
class OpenAITTSClient : public QObject {
    Q_OBJECT

public:
    explicit OpenAITTSClient(QObject *parent = nullptr);
    ~OpenAITTSClient() override;

    void synthesize(const QString &text, const QString &format = "mp3");
    void playLastResponse();
    void stop();
    bool isPlaying() const;
    void setBaseUrl(const QString &url);
    void setApiKey(const QString &key);
    void setVoice(const QString &voice);
    void setModel(const QString &model);
    void setInstructions(const QString &);
    void setFormat(const QString &format);

public slots:
    void reset();

signals:
    /**
   * @brief 音声再生開始
   */
    void playbackStarted();

    /**
   * @brief 音声再生完了
   */
    void playbackFinished();

    /**
   * @brief エラー発生
   * @param error エラーメッセージ
   */
    void errorOccurred(const QString &error);

    /**
   * @brief ステータス更新
   * @param status ステータスメッセージ
   */
    void statusChanged(const QString &status);

private slots:
    void onSynthesizeFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QNetworkReply *m_currentReply;
    std::unique_ptr<QTemporaryFile> m_tempFile;
    QString m_baseUrl;
    QString m_apiKey;
    QString m_voice;
    QString m_model;
    QString m_instructions;
    QString m_format;
    bool m_isPlaying;

    void sendSynthesizeRequest(const QString &text);
};
