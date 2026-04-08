#pragma once

#include <QString>
#include <QStringListModel>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class ChatWidget;
}
QT_END_NAMESPACE

class LMStudioClient;

/**
 * @brief チャットUIウィジェットクラス
 * メッセージ表示、テキスト入力、送信機能を提供する
 */
class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget() override;

private slots:
    void onSendClicked();
    void onReplyReceived(const QString &reply);
    void onErrorOccurred(const QString &error);

private:
    Ui::ChatWidget *ui;
    LMStudioClient *m_client;
    QStringListModel *m_model;

    void appendMessage(const QString &role, const QString &message);
};
