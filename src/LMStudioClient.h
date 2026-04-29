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
   * @param history 送信するチャット履歴（system プロンプトを除く user /
   *                assistant メッセージの JSON 配列）。最後の要素が今回送る
   *                ユーザーメッセージとなる。
   *
   * チャット履歴はクライアント内部に保持しない。呼び出し側
   * （MainWindow の表示モデル）を Single Source of Truth として、
   * 毎回その時点の履歴を渡す。
   */
    void sendRequest(const QJsonArray &history);

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
    QString m_baseUrl;
    SystemPromptProfile m_activeProfile;
    bool m_isConnectionTest;

    void handleNetworkError(QNetworkReply *reply);
};
