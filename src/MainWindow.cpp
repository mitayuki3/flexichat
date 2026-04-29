#include "MainWindow.h"
#include "ProfileManager.h"
#include "ui_MainWindow.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>

/**
 * @brief コンストラクタ
 * UI のセットアップを行う
 * @param profileManager プロファイルマネージャー
 */
MainWindow::MainWindow(ProfileManager *profileManager, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    m_profileManager(profileManager), m_model(new QStringListModel(this)),
    m_lastAssistantMessage(""), m_pendingTtsText(""),
    m_profileCommitTimer(new QTimer(this)) {
    ui->setupUi(this);

    // プロファイル編集の保存をデバウンスするタイマー
    m_profileCommitTimer->setSingleShot(true);
    m_profileCommitTimer->setInterval(600);
    connect(m_profileCommitTimer, &QTimer::timeout, this,
            &MainWindow::commitProfileEdits);

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
 * 終了前に未保存のプロファイル編集をフラッシュする
 */
MainWindow::~MainWindow() {
    if (m_profileCommitTimer->isActive()) {
        m_profileCommitTimer->stop();
        commitProfileEdits();
    }
    delete ui;
}

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

    connect(ui->sendButton, &QPushButton::clicked, this,
            &MainWindow::onSendClicked);
    connect(ui->inputField, &QLineEdit::returnPressed, this,
            &MainWindow::onSendClicked);

    // TTS ボタンの接続
    connect(ui->playTtsButton, &QPushButton::clicked, this,
            &MainWindow::onPlayTtsClicked);

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
    connect(ui->ttsVoiceCombo, &QComboBox::currentTextChanged, this,
            &MainWindow::voiceChanged);

    // TTS リスト
    connect(ui->ttsListWidget, &QListWidget::currentRowChanged, this,
            &MainWindow::onTtsListRowChanged);
    connect(ui->ttsListWidget, &QListWidget::activated, this,
            &MainWindow::onTtsListActivated);

    // プロファイル設定エディタ
    // editingFinished（フォーカスアウト / Enter）では即時保存し、
    // それ以外（連続するクリックや入力）はタイマーでデバウンス保存する
    connect(ui->profileNameEdit, &QLineEdit::editingFinished, this,
            &MainWindow::commitProfileEdits);
    connect(ui->profilePromptEdit, &QPlainTextEdit::textChanged, this,
            &MainWindow::scheduleProfileCommit);
    connect(ui->profileTemperatureSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double) { scheduleProfileCommit(); });
    connect(ui->profileTemperatureSpin, &QDoubleSpinBox::editingFinished, this,
            &MainWindow::commitProfileEdits);
    connect(ui->profileMaxTokensSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { scheduleProfileCommit(); });
    connect(ui->profileMaxTokensSpin, &QSpinBox::editingFinished, this,
            &MainWindow::commitProfileEdits);

    // プロファイルの追加・ゴミ箱
    connect(ui->addProfileButton, &QPushButton::clicked, this,
            &MainWindow::onAddProfileClicked);
    connect(ui->trashProfileButton, &QPushButton::clicked, this,
            &MainWindow::onTrashProfileClicked);

    // ゴミ箱タブ
    connect(ui->restoreTrashButton, &QPushButton::clicked, this,
            &MainWindow::onRestoreTrashedClicked);
    connect(ui->emptyTrashButton, &QPushButton::clicked, this,
            &MainWindow::onEmptyTrashClicked);
    connect(ui->trashListWidget, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *) { onRestoreTrashedClicked(); });
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
    populateTrashList();
    updateTrashButton();
}

/**
 * @brief ゴミ箱タブのリストを再構築する
 * 各アイテムにはプロファイル ID を Qt::UserRole として持たせる
 */
void MainWindow::populateTrashList() {
    auto trashed = m_profileManager->getTrashedProfiles();
    ui->trashListWidget->clear();
    for (const auto &p : trashed) {
        auto *item = new QListWidgetItem(p.displayName(), ui->trashListWidget);
        item->setData(Qt::UserRole, p.id);
    }
}

/**
 * @brief ゴミ箱関連ボタンとタブタイトルの状態を更新
 */
void MainWindow::updateTrashButton() {
    int n = m_profileManager->getTrashedCount();

    // タブタイトルに件数を表示
    int trashTabIndex = ui->tabWidget->indexOf(ui->trashTab);
    if (trashTabIndex >= 0) {
        ui->tabWidget->setTabText(
            trashTabIndex, n > 0 ? QString("ゴミ箱 (%1)").arg(n) : "ゴミ箱");
    }

    ui->emptyTrashButton->setEnabled(n > 0);
    ui->restoreTrashButton->setEnabled(n > 0);

    // 残るプロファイルが 1 つだけならゴミ箱に入れさせない
    bool canTrash = m_profileManager->getAllProfiles().size() > 1;
    ui->trashProfileButton->setEnabled(canTrash &&
                                       !m_displayedProfileId.isEmpty());
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
    populateProfileCombo();
    // 自身の編集による更新ではエディタを再読み込みしない
    // それ以外（プロファイル切替や外部編集）ではエディタを同期する
    if (!m_committingFromEditor) {
        loadProfileIntoEditor(profile);
    }
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
 * @brief プロファイルの内容をエディタに読み込む
 * 現在表示中のプロファイルに未保存の編集があれば先にフラッシュする
 */
void MainWindow::loadProfileIntoEditor(const SystemPromptProfile &profile) {
    if (m_profileCommitTimer->isActive()) {
        m_profileCommitTimer->stop();
        commitProfileEdits();
    }
    m_loadingProfileFields = true;
    m_displayedProfileId = profile.id;
    ui->profileNameEdit->setText(profile.name);
    ui->profilePromptEdit->setPlainText(profile.prompt);
    ui->profileTemperatureSpin->setValue(profile.temperature);
    ui->profileMaxTokensSpin->setValue(profile.maxTokens);
    ui->profileSettingsGroupBox->setEnabled(!profile.id.isEmpty());
    m_loadingProfileFields = false;
    updateTrashButton();
}

/**
 * @brief 編集の保存をデバウンスする
 */
void MainWindow::scheduleProfileCommit() {
    if (m_loadingProfileFields || m_displayedProfileId.isEmpty()) {
        return;
    }
    m_profileCommitTimer->start();
}

/**
 * @brief エディタの内容をプロファイルに反映する
 * 保留中のタイマーがあれば停止する（即時フラッシュ）
 */
void MainWindow::commitProfileEdits() {
    m_profileCommitTimer->stop();
    if (m_loadingProfileFields || m_displayedProfileId.isEmpty()) {
        return;
    }
    auto *current = m_profileManager->getProfileById(m_displayedProfileId);
    if (!current) {
        return;
    }

    SystemPromptProfile updated = *current;
    QString name = ui->profileNameEdit->text().trimmed();
    if (name.isEmpty()) {
        // 空の名前は保存しない（既存名を保持）
        ui->profileNameEdit->setText(updated.name);
    } else {
        updated.name = name;
    }
    updated.prompt = ui->profilePromptEdit->toPlainText();
    updated.temperature = ui->profileTemperatureSpin->value();
    updated.maxTokens = ui->profileMaxTokensSpin->value();

    if (updated.name == current->name && updated.prompt == current->prompt &&
        qFuzzyCompare(updated.temperature, current->temperature) &&
        updated.maxTokens == current->maxTokens) {
        return;
    }

    m_committingFromEditor = true;
    m_profileManager->updateProfile(updated);
    m_committingFromEditor = false;
}

/**
 * @brief プロファイルを新規追加
 */
void MainWindow::onAddProfileClicked() {
    // 編集中の内容を確実に保存してから追加する
    if (m_profileCommitTimer->isActive()) {
        m_profileCommitTimer->stop();
        commitProfileEdits();
    }

    SystemPromptProfile p =
        SystemPromptProfile::createDefault("", "新規プロファイル");
    m_profileManager->addProfile(p);

    // 追加されたプロファイルをアクティブにする（末尾に追加されている）
    auto profiles = m_profileManager->getAllProfiles();
    if (!profiles.isEmpty()) {
        m_profileManager->setActiveProfile(profiles.last().id);
    }

    // 名前入力にフォーカスを当てて即編集できるようにする
    ui->profileNameEdit->setFocus();
    ui->profileNameEdit->selectAll();
}

/**
 * @brief 現在のプロファイルをゴミ箱に移す（確認なし）
 */
void MainWindow::onTrashProfileClicked() {
    QString id = m_profileManager->getActiveProfileId();
    if (id.isEmpty()) {
        return;
    }
    if (m_profileManager->getAllProfiles().size() <= 1) {
        ui->statusbar->showMessage(
            "最後のプロファイルはゴミ箱に入れられません", 3000);
        return;
    }

    if (m_profileCommitTimer->isActive()) {
        m_profileCommitTimer->stop();
    }

    auto active = m_profileManager->getActiveProfile();
    m_profileManager->trashProfile(id);
    ui->statusbar->showMessage(
        QString("「%1」をゴミ箱に移動しました").arg(active.displayName()),
        3000);
}

/**
 * @brief ゴミ箱を空にする（確認なし・即時削除）
 */
void MainWindow::onEmptyTrashClicked() {
    int n = m_profileManager->getTrashedCount();
    if (n <= 0) {
        return;
    }
    m_profileManager->emptyTrash();
    ui->statusbar->showMessage(
        QString("ゴミ箱を空にしました (%1 件削除)").arg(n), 3000);
}

/**
 * @brief 選択中のゴミ箱内プロファイルを復元する
 */
void MainWindow::onRestoreTrashedClicked() {
    auto *item = ui->trashListWidget->currentItem();
    if (!item) {
        return;
    }
    QString id = item->data(Qt::UserRole).toString();
    if (id.isEmpty()) {
        return;
    }
    auto *p = m_profileManager->getProfileById(id);
    QString name = p ? p->displayName() : id;
    m_profileManager->restoreProfile(id);
    ui->statusbar->showMessage(
        QString("「%1」をゴミ箱から戻しました").arg(name), 3000);
}

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

    // プロファイル設定エディタにアクティブプロファイルを読み込み
    loadProfileIntoEditor(m_profileManager->getActiveProfile());
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

    // テキストを保存
    m_pendingTtsText = text.startsWith("AI: ") ? text.mid(4) : text;

    // シグナルで TTS クライアントに送信（保存したテキストを使用）
    emit synthesizeRequested(m_pendingTtsText);
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
    if (ui->forEachLineCheckBox->isChecked()) {
        QStringList list = m_pendingTtsText.split("\n");
        // 前後の空白を除去する
        for (QString &s : list) {
            s = s.trimmed();
        }
        // 空文字列を除去する
        list.removeAll(QString());
        emit synthesizeMultipleRequested(list);
    } else {
        emit synthesizeRequested(m_pendingTtsText);
    }
}

/**
 * @brief TTS ボタンの同期
 * 再生中かどうかでボタン状態を切り替え（シグナル経由）
 */
void MainWindow::syncTtsButtons() {
    // TTS クライアントのステータスは main.cpp からシグナルで更新
    // 再生中でなければ再生ボタン有効
    ui->playTtsButton->setEnabled(m_pendingTtsText.isEmpty() == false);
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

/**
 * @brief TTS 生成完了時
 * @param filePath 生成されたファイルパス
 */
void MainWindow::appendTtsOutput(QString const &filePath) {
    ui->ttsListWidget->addItem(filePath);
}

void MainWindow::onTtsListRowChanged(int row) {
    if (QListWidgetItem *item = ui->ttsListWidget->item(row)) {
        emit ttsFileSelected(item->text());
    }
}

/**
 * @brief TTS リストアクティベート時
 * @param index アクティベートされたインデックス
 */
void MainWindow::onTtsListActivated(const QModelIndex &index) {
    if (!index.isValid()) {
        return;
    }
    if (QListWidgetItem *item = ui->ttsListWidget->itemFromIndex(index)) {
        emit ttsFileActivated(item->text());
    }
}
