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

    /**
   * @brief チャットリクエストの送信
   * @param message ユーザーのメッセージ
   */
    void sendRequest(const QString &message);

    /**
   * @brief プロファイルの設定
   * @param profile システムプロンプトプロファイル
   */
    void setProfile(const SystemPromptProfile &profile);

    /**
   * @brief ベースURLの設定
   */
    void setBaseUrl(const QString &url);

    /**
   * @brief チャット履歴のリセット
   */
    void resetChatHistory();

    /**
   * @brief チャット履歴の置き換え
   * @param history user / assistant メッセージで構成された JSON 配列
   *
   * UI 側でチャットアイテムが編集・削除されたときに呼び出し、
   * 次回以降のリクエストで送信される履歴を UI と同期する。
   */
    void setChatHistory(const QJsonArray &history);

    /**
   * @brief 接続テスト
   */
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
    QJsonArray m_chatHistory;
    QString m_baseUrl;
    SystemPromptProfile m_activeProfile;
    bool m_isConnectionTest;

    void handleNetworkError(QNetworkReply *reply);
};
