#pragma once

#include <QWidget>
#include <QString>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class LMStudioClient;

/**
 * @brief チャットUIウィジェットクラス
 * メッセージ表示、テキスト入力、送信機能を提供する
 */
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget() override;

private slots:
    void onSendClicked();
    void onReplyReceived(const QString &reply);
    void onErrorOccurred(const QString &error);

private:
    QTextEdit *m_chatDisplay;
    QLineEdit *m_inputField;
    QPushButton *m_sendButton;
    QVBoxLayout *m_layout;
    LMStudioClient *m_client;

    void setupUI();
    void appendMessage(const QString &role, const QString &message);
};