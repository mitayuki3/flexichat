#pragma once

#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class LMStudioClient;

/**
 * @brief メインウィンドウクラス
 * アプリケーションのメインウィンドウを提供し、チャット機能を提供する
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(LMStudioClient *client, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSendClicked();
    void onReplyReceived(const QString &reply);
    void onErrorOccurred(const QString &error);
    void onApiRequestStarted();
    void onApiRequestFinished();

private:
    Ui::MainWindow *ui;
    LMStudioClient *m_client;
    QStringListModel *m_model;

    void setupUI();
    void connectSignals();
    void appendMessage(const QString &role, const QString &message);
};
