#include "MainWindow.h"
#include "LMStudioClient.h"
#include "ProfileManager.h"
#include "SettingsDialog.h"
#include "ui_MainWindow.h"
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QToolBar>

/**
 * @brief コンストラクタ
 * UIのセットアップを行う
 */
MainWindow::MainWindow(LMStudioClient *client, ProfileManager *profileManager,
                       QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_client(client),
      m_profileManager(profileManager), m_model(new QStringListModel(this)),
      m_profileCombo(nullptr) {
    ui->setupUi(this);

    // モデルのセットアップ
    ui->chatDisplay->setModel(m_model);

    setupUI();
    setupToolbar();
    connectSignals();
    populateProfileCombo();

    // 初期プロファイルをクライアントに設定
    m_client->setProfile(m_profileManager->getActiveProfile());

    // ProfileManagerのシグナルを接続
    connect(m_profileManager, &ProfileManager::activeProfileChanged, this,
            &MainWindow::onProfileChanged);
    connect(m_profileManager, &ProfileManager::profileListChanged, this,
            &MainWindow::onProfileListChanged);

    // LMStudioClientのシグナルを接続
    connect(m_client, &LMStudioClient::replyReceived, this,
            &MainWindow::onReplyReceived);
    connect(m_client, &LMStudioClient::errorOccurred, this,
            &MainWindow::onErrorOccurred);
    connect(m_client, &LMStudioClient::requestStarted, this,
            &MainWindow::onApiRequestStarted);
    connect(m_client, &LMStudioClient::requestCompleted, this,
            &MainWindow::onApiRequestFinished);
}

/**
 * @brief デストラクタ
 */
MainWindow::~MainWindow() { delete ui; }

/**
 * @brief シグナル接続の設定
 */
void MainWindow::connectSignals() {
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "バージョン情報",
                           "FlexiChat v1.0.0\n\nQt + CMake AIチャットアプリ");
    });

    connect(ui->actionSettings, &QAction::triggered, this,
            &MainWindow::openSettings);

    connect(ui->sendButton, &QPushButton::clicked, this,
            &MainWindow::onSendClicked);
    connect(ui->inputField, &QLineEdit::returnPressed, this,
            &MainWindow::onSendClicked);
}

/**
 * @brief ツールバーのセットアップ
 */
void MainWindow::setupToolbar() {
    auto *toolbar = ui->profileToolBar;

    // プロファイル切り替えコンボボックス
    auto *label = new QLabel("プロファイル: ");
    toolbar->addWidget(label);

    m_profileCombo = new QComboBox();
    m_profileCombo->setMinimumWidth(200);
    toolbar->addWidget(m_profileCombo);

    connect(m_profileCombo, QOverload<int>::of(&QComboBox::activated), this,
            &MainWindow::onProfileComboActivated);
}

/**
 * @brief プロファイルコンボボックスの更新
 */
void MainWindow::populateProfileCombo() {
    QString currentId = m_profileCombo->currentData().toString();
    m_profileCombo->clear();

    auto profiles = m_profileManager->getAllProfiles();
    int activeIdx = 0;

    for (int i = 0; i < profiles.size(); ++i) {
        const auto &p = profiles[i];
        m_profileCombo->addItem(p.displayName(), p.id);
        if (p.id == m_profileManager->getActiveProfileId()) {
            activeIdx = i;
        }
    }

    m_profileCombo->setCurrentIndex(activeIdx);
}

/**
 * @brief プロファイル切り替えの確認
 */
bool MainWindow::confirmProfileSwitch() {
    if (m_model->rowCount() == 0) {
        return true;
    }

    auto result = QMessageBox::question(
        this, "プロファイル切り替え",
        "プロファイルを切り替えると、チャット履歴がクリアされます。\n"
        "続行しますか？",
        QMessageBox::Yes | QMessageBox::No);

    return result == QMessageBox::Yes;
}

/**
 * @brief プロファイルコンボボックスの選択変更
 */
void MainWindow::onProfileComboActivated(int index) {
    QString id = m_profileCombo->itemData(index).toString();
    if (id.isEmpty() || id == m_profileManager->getActiveProfileId()) {
        return;
    }

    if (!confirmProfileSwitch()) {
        int currentIdx =
            m_profileCombo->findData(m_profileManager->getActiveProfileId());
        if (currentIdx >= 0) {
            m_profileCombo->setCurrentIndex(currentIdx);
        }
        return;
    }

    m_client->resetChatHistory();
    m_model->setStringList({});
    m_profileManager->setActiveProfile(id);
}

/**
 * @brief 設定ダイアログを開く
 */
void MainWindow::openSettings() {
    SettingsDialog dialog(m_profileManager, m_client, this);
    dialog.exec();
    populateProfileCombo();
}

/**
 * @brief プロファイル変更時の処理
 */
void MainWindow::onProfileChanged(const SystemPromptProfile &profile) {
    Q_UNUSED(profile);
    populateProfileCombo();
    ui->statusbar->showMessage(
        "プロファイル: " + m_profileManager->getActiveProfile().displayName(),
        3000);
}

/**
 * @brief プロファイルリスト変更時の処理
 */
void MainWindow::onProfileListChanged() { populateProfileCombo(); }

/**
 * @brief 送信ボタンがクリックされたときの処理
 */
void MainWindow::onSendClicked() {
    QString message = ui->inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    appendMessage("user", message);

    ui->inputField->clear();
    ui->inputField->setEnabled(false);
    ui->sendButton->setEnabled(false);

    m_client->sendRequest(message);
}

/**
 * @brief AIからの応答を受信したときの処理
 */
void MainWindow::onReplyReceived(const QString &reply) {
    appendMessage("assistant", reply);

    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
}

/**
 * @brief エラーが発生したときの処理
 */
void MainWindow::onErrorOccurred(const QString &error) {
    appendMessage("error", "エラー: " + error);

    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
}

/**
 * @brief APIリクエスト開始時のステータス更新
 */
void MainWindow::onApiRequestStarted() {
    ui->statusbar->showMessage("応答を生成中...");
}

/**
 * @brief APIリクエスト完了時のステータス更新
 */
void MainWindow::onApiRequestFinished() {
    ui->statusbar->showMessage("準備完了", 3000);
}

/**
 * @brief UIのセットアップ
 */
void MainWindow::setupUI() { ui->statusbar->showMessage("準備完了"); }

/**
 * @brief メッセージをチャット表示に追加
 * @param role 役割（user/assistant/error）
 * @param message メッセージ内容
 */
void MainWindow::appendMessage(const QString &role, const QString &message) {
    QString displayMessage;
    if (role == "user") {
        displayMessage = QString("You: %1").arg(message);
    } else if (role == "assistant") {
        displayMessage = QString("AI: %1").arg(message);
    } else {
        displayMessage = message;
    }

    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setData(m_model->index(row), displayMessage);

    ui->chatDisplay->scrollToBottom();
}
