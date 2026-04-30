#pragma once

#include "SystemPromptProfile.h"
#include <QJsonArray>
#include <QNetworkReply>
#include <QObject>

class QNetworkAccessManager;

/**
 * @brief LM Studio API通信クライアント
 * LM StudioのローカルAPIサーバーと通信してチャット応答を取得する
 */
class LMStudioClient : public QObject {
    Q_OBJECT

public:
    explicit LMStudioClient(QObject *parent = nullptr);
    ~LMStudioClient() override;

    void sendRequest(const QJsonArray &history);
    void setProfile(const SystemPromptProfile &profile);
    void setBaseUrl(const QString &url);
    void testConnection();

signals:
    void replyReceived(const QString &reply);
    void errorOccurred(const QString &error);
    void requestStarted();
    void requestCompleted();
    void connectionTestResult(bool success, const QString &message);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onConnectionTestFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_baseUrl;
    SystemPromptProfile m_activeProfile;
    bool m_isConnectionTest;

    void handleNetworkError(QNetworkReply *reply);
};
