#include "MainWindow.h"
#include "ProfileManager.h"
#include "SettingsDialog.h"
#include "ui_MainWindow.h"
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>

/**
 * @brief コンストラクタ
 * UI のセットアップを行う
 * @param profileManager プロファイルマネージャー
 */
MainWindow::MainWindow(ProfileManager *profileManager, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    m_profileManager(profileManager), m_model(new QStringListModel(this)),
    m_lastAssistantMessage(""), m_pendingTtsText("") {
    ui->setupUi(this);

    // モデルのセットアップ
    ui->chatDisplay->setModel(m_model);

    setupUI();
    populateProfileCombo();
    connectSignals();

    // 初期プロファイルをクライアントに設定（シグナル経由）
    emit profileChangeRequested(m_profileManager->getActiveProfileId());
}

/**
 * @brief デストラクタ
 */
MainWindow::~MainWindow() { delete ui; }

/**
 * @brief シグナル接続の設定
 */
void MainWindow::connectSignals() {
    // 自動再生チェックボックス
    connect(ui->autoplayCheckBox, &QCheckBox::checkStateChanged, this,
            [this](Qt::CheckState state) {
                emit autoplayChanged(state == Qt::Checked);
    });
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->chatDisplay, &QListView::clicked, this,
            &MainWindow::onChatDisplayClicked);

    connect(ui->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this, "バージョン情報",
            "AI チャットアプリ FlexiChat v1.0.0\nImplemented with Qt");
    });

    connect(ui->actionSettings, &QAction::triggered, this,
            &MainWindow::openSettingsRequested);

    connect(ui->sendButton, &QPushButton::clicked, this,
            &MainWindow::onSendClicked);
    connect(ui->inputField, &QLineEdit::returnPressed, this,
            &MainWindow::onSendClicked);

    // TTS ボタンの接続
    connect(ui->playTtsButton, &QPushButton::clicked, this,
            &MainWindow::onPlayTtsClicked);
    connect(ui->stopTtsButton, &QPushButton::clicked, this,
            &MainWindow::onStopTtsClicked);

    connect(ui->chatDisplay, &QListView::activated, this,
            [this](const QModelIndex &index) {
                this->m_pendingTtsText = this->m_model->data(index).toString();
        this->syncTtsButtons();
    });

    connect(ui->profileCombo, QOverload<int>::of(&QComboBox::activated), this,
            &MainWindow::onProfileComboActivated);

    // TTS タブ
    connect(ui->ttsGenerateButton, &QPushButton::clicked, this,
            &MainWindow::generateTtsSpeech);
    connect(ui->ttsPlayButton, &QPushButton::clicked, this,
            &MainWindow::ttsPlayRequested);
}

/**
 * @brief プロファイルコンボボックスの更新
 */
void MainWindow::populateProfileCombo() {
    QComboBox *profileCombo = ui->profileCombo;
    QString currentId = profileCombo->currentData().toString();
    profileCombo->clear();

    auto profiles = m_profileManager->getAllProfiles();
    int activeIdx = 0;

    for (int i = 0; i < profiles.size(); ++i) {
        const auto &p = profiles[i];
        profileCombo->addItem(p.displayName(), p.id);
        if (p.id == m_profileManager->getActiveProfileId()) {
            activeIdx = i;
        }
    }

    profileCombo->setCurrentIndex(activeIdx);
}

/**
 * @brief プロファイルコンボボックスの選択変更
 */
void MainWindow::onProfileComboActivated(int index) {
    QString id = ui->profileCombo->itemData(index).toString();
    if (id.isEmpty() || id == m_profileManager->getActiveProfileId()) {
        return;
    }

    // 履歴は保持したまま、プロファイルのみ切り替え
    m_profileManager->setActiveProfile(id);
}

/**
 * @brief プロファイル変更時の処理
 */
void MainWindow::onProfileChanged(const SystemPromptProfile &profile) {
    Q_UNUSED(profile);
    populateProfileCombo();
    emit profileChangeRequested(m_profileManager->getActiveProfileId());
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

    emit requestSend(message);
}

/**
 * @brief AI からの応答を受信したときの処理
 */
void MainWindow::onReplyReceived(const QString &reply) {
    appendMessage("assistant", reply);
    m_lastAssistantMessage = reply;

    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();

    syncTtsButtons();
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
 * @brief API リクエスト開始時のステータス更新
 */
void MainWindow::onApiRequestStarted() {
    ui->statusbar->showMessage("応答を生成中...");
}

/**
 * @brief API リクエスト完了時のステータス更新
 */
void MainWindow::onApiRequestFinished() {
    ui->statusbar->showMessage("準備完了", 3000);
}

/**
 * @brief UI のセットアップ
 */
void MainWindow::setupUI() {
    ui->statusbar->showMessage("準備完了");

    // TTS タブの初期化
    // 保存されたモデルとボイスを選択状態に
    QString savedModel = m_profileManager->getTtsModel();
    for (int i = 0; i < ui->ttsModelListWidget->count(); ++i) {
        if (ui->ttsModelListWidget->item(i)->text() == savedModel) {
            ui->ttsModelListWidget->setCurrentRow(i);
            break;
        }
    }
    QString savedVoice = m_profileManager->getTtsVoice();
    ui->ttsVoiceCombo->setCurrentText(savedVoice);

    // 保存された自動再生設定をチェックボックスに反映
    ui->autoplayCheckBox->setChecked(m_profileManager->getTtsAutoPlay());
}

/**
 * @brief メッセージをチャット表示に追加
 * @param role 役割 (user/assistant/error)
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

/**
 * @brief 音声再生ボタンクリック時
 * 選択中のメッセージを TTS で再生（シグナル経由）
 */
void MainWindow::onPlayTtsClicked() {
    QModelIndexList selected =
        ui->chatDisplay->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        onErrorOccurred("チャット履歴から再生したい発言を選択してください。");
        return;
    }

    QString text = m_model->data(selected.first()).toString();
    if (text.isEmpty()) {
        return;
    }

    // AI の発言のみ再生可能
    if (text.startsWith("You:")) {
        onErrorOccurred(
            "You の発言は再生できません。AI の発言を選択してください。");
        return;
    }

    ui->statusbar->showMessage("TTS 再生中...");

    // テキストを保存
    m_pendingTtsText = text.startsWith("AI: ") ? text.mid(4) : text;

    // シグナルで TTS クライアントに送信（保存したテキストを使用）
    emit synthesizeRequested(m_pendingTtsText);
}

/**
 * @brief 音声停止ボタンクリック時
 * 再生中の TTS を停止（シグナル経由）
 */
void MainWindow::onStopTtsClicked() {
    m_pendingTtsText = "";
    emit stopTtsRequested();
}

/**
 * @brief チャットディスプレイクリック時
 * 選択ボタンの同期
 */
void MainWindow::onChatDisplayClicked(const QModelIndex &index) {
    Q_UNUSED(index);
    syncTtsButtons();
}

/**
 * @brief TTS タブの「生成」ボタンクリック時
 */
void MainWindow::generateTtsSpeech() {
    m_pendingTtsText = ui->ttsTextEdit->toPlainText().trimmed();
    if (m_pendingTtsText.isEmpty()) {
        onErrorOccurred("TTS 入力が空です。テキストを入力してください");
        return;
    }
    emit synthesizeRequested(m_pendingTtsText);
}

/**
 * @brief TTS ボタンの同期
 * 再生中かどうかでボタン状態を切り替え（シグナル経由）
 */
void MainWindow::syncTtsButtons() {
    // TTS クライアントのステータスは main.cpp からシグナルで更新
    // 再生中でなければ再生ボタン有効
    ui->playTtsButton->setEnabled(m_pendingTtsText.isEmpty() == false);
    ui->stopTtsButton->setEnabled(false);
}

/**
 * @brief pendingTtsText を返す
 */
QString MainWindow::getPendingTtsText() const { return m_pendingTtsText; }

/**
 * @brief ステータスバーを更新
 * @param status ステータスメッセージ
 */
void MainWindow::showStatusMessage(const QString &status) {
    ui->statusbar->showMessage(status);
}
