#pragma once

#include <QMainWindow>

class ChatWidget;

/**
 * @brief メインウィンドウクラス
 * アプリケーションのメインウィンドウを提供し、チャットウィジェットを配置する
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    ChatWidget *m_chatWidget;

    void setupUI();
    void setupMenuBar();
};