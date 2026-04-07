#include "MainWindow.h"
#include "ChatWidget.h"
#include "ui_MainWindow.h"
#include <QMessageBox>

/**
 * @brief コンストラクタ
 * UIのセットアップを行う
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_chatWidget(nullptr) {
    ui->setupUi(this);
    connectSignals();
    setupUI();
}

/**
 * @brief デストラクタ
 */
MainWindow::~MainWindow() { delete ui; }

/**
 * @brief シグナル接続の設定
 */
void MainWindow::connectSignals() {
    // 終了アクションの接続
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    // バージョン情報アクションの接続
    connect(ui->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "バージョン情報",
                           "FlexiChat v1.0.0\n\nQt + CMake AIチャットアプリ");
    });
}

/**
 * @brief UIのセットアップ
 */
void MainWindow::setupUI() {
    // チャットウィジェットの作成
    m_chatWidget = new ChatWidget(this);
    ui->chatContainer->layout()->addWidget(m_chatWidget);
}
