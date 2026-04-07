#pragma once

#include <QObject>
#include <QNetworkReply>
#include <QJsonArray>

class QNetworkAccessManager;

/**
 * @brief LM Studio API通信クライアント
 * LM StudioのローカルAPIサーバーと通信してチャット応答を取得する
 */
class LMStudioClient : public QObject
{
    Q_OBJECT

public:
    explicit LMStudioClient(QObject *parent = nullptr);
    ~LMStudioClient() override;

    /**
     * @brief チャットリクエストの送信
     * @param message ユーザーのメッセージ
     */
    void sendRequest(const QString &message);

signals:
    /**
     * @brief 応答を受信したときに発行されるシグナル
     * @param reply 応答メッセージ
     */
    void replyReceived(const QString &reply);

    /**
     * @brief エラーが発生したときに発行されるシグナル
     * @param error エラーメッセージ
     */
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QJsonArray m_chatHistory;
    QString m_baseUrl;

    void handleNetworkError(QNetworkReply *reply);
};