#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ChatWidget;

/**
 * @brief メインウィンドウクラス
 * アプリケーションのメインウィンドウを提供し、チャットウィジェットを配置する
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    ChatWidget *m_chatWidget;

    void setupUI();
    void connectSignals();
};
