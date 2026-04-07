#include "MainWindow.h"
#include "ChatWidget.h"
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

/**
 * @brief コンストラクタ
 * UIのセットアップを行う
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_chatWidget(nullptr)
{
    setWindowTitle("FlexiChat - LM Studio");
    resize(800, 600);

    setupMenuBar();
    setupUI();
}

/**
 * @brief デストラクタ
 */
MainWindow::~MainWindow() = default;

/**
 * @brief メニューバーのセットアップ
 */
void MainWindow::setupMenuBar()
{
    // ファイルメニュー
    auto *fileMenu = menuBar()->addMenu("ファイル");
    auto *exitAction = fileMenu->addAction("終了");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // ヘルプメニュー
    auto *helpMenu = menuBar()->addMenu("ヘルプ");
    auto *aboutAction = helpMenu->addAction("バージョン情報");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "バージョン情報", "FlexiChat v1.0.0\n\nQt + CMake AIチャットアプリ");
    });
}

/**
 * @brief UIのセットアップ
 */
void MainWindow::setupUI()
{
    // チャットウィジェットの作成
    m_chatWidget = new ChatWidget(this);
    setCentralWidget(m_chatWidget);
}