#include "MainWindow.h"
#include "ProfileManager.h"
#include "ui_MainWindow.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <algorithm>

namespace {

constexpr QLatin1String kUserPrefix("You: ");
constexpr QLatin1String kAssistantPrefix("AI: ");

/**
 * @brief role に対応する表示プレフィックスを返す
 *
 * 表示への挿入と表示からの抽出で同じ対応表を使うための単一の参照点。
 */
QLatin1String prefixForRole(ChatMessage::Role role) {
    switch (role) {
    case ChatMessage::Role::User:
        return kUserPrefix;
    case ChatMessage::Role::Assistant:
        return kAssistantPrefix;
    }
    Q_UNREACHABLE();
}

/**
 * @brief 表示モデルの末尾に 1 行追加する
 */
void appendLine(QStringListModel *model, const QString &line) {
    const int row = model->rowCount();
    model->insertRow(row);
    model->setData(model->index(row), line);
}

/**
 * @brief role 付きのチャット行を表示モデルに追加する
 */
void appendChatLine(QStringListModel *model, ChatMessage::Role role,
                    const QString &message) {
    appendLine(model, prefixForRole(role) + message);
}

/**
 * @brief role を持たない生のメッセージ行（エラー等）を追加する
 */
void appendRawLine(QStringListModel *model, const QString &message) {
    appendLine(model, message);
}

/**
 * @brief チャット表示モデルの内容から API 送信用の ChatHistory を作る
 *
 * 表示モデルの各行を `kUserPrefix` で始まれば user、`kAssistantPrefix`
 * で始まれば assistant メッセージとして扱う。エラー行など、それ以外の
 * 行は履歴に含めない。表示形式と履歴データ構造の境界を MainWindow 内に
 * 閉じ込めるためのフリー関数。
 */
ChatHistory chatHistoryFromListModel(const QStringListModel *model) {
    ChatHistory history;
    if (!model) {
        return history;
    }
    const int rowCount = model->rowCount();
    history.reserve(rowCount);
    for (int i = 0; i < rowCount; ++i) {
        const QString text = model->data(model->index(i)).toString();
        if (text.startsWith(kUserPrefix)) {
            history.append({ChatMessage::Role::User, text.mid(kUserPrefix.size())});
        } else if (text.startsWith(kAssistantPrefix)) {
            history.append(
                {ChatMessage::Role::Assistant, text.mid(kAssistantPrefix.size())});
        }
        // それ以外（エラー等）は履歴に含めない
    }
    return history;
}

} // namespace

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
    connect(ui->chatDisplay, &QWidget::customContextMenuRequested, this,
            &MainWindow::onChatDisplayContextMenu);

    connect(ui->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this, "バージョン情報",
            "AI チャットアプリ FlexiChat v1.0.0\nImplemented with Qt");
    });

    connect(ui->sendButton, &QPushButton::clicked, this,
            &MainWindow::onSendClicked);
    connect(ui->inputField, &QLineEdit::returnPressed, this,
            &MainWindow::onSendClicked);

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
        ui->tabWidget->setTabText(trashTabIndex,
                                  n > 0 ? QString("ゴミ箱 (%1)").arg(n) : "ゴミ箱");
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
        ui->statusbar->showMessage("最後のプロファイルはゴミ箱に入れられません",
                                   3000);
        return;
    }

    if (m_profileCommitTimer->isActive()) {
        m_profileCommitTimer->stop();
    }

    auto active = m_profileManager->getActiveProfile();
    m_profileManager->trashProfile(id);
    ui->statusbar->showMessage(
        QString("「%1」をゴミ箱に移動しました").arg(active.displayName()), 3000);
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
    ui->statusbar->showMessage(QString("ゴミ箱を空にしました (%1 件削除)").arg(n),
                               3000);
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
    ui->statusbar->showMessage(QString("「%1」をゴミ箱から戻しました").arg(name),
                               3000);
}

/**
 * @brief 送信ボタンがクリックされたときの処理
 */
void MainWindow::onSendClicked() {
    QString message = ui->inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    appendUserMessage(message);

    ui->inputField->clear();
    ui->inputField->setEnabled(false);
    ui->sendButton->setEnabled(false);

    // チャット表示モデルを Single Source of Truth として、
    // 現時点のチャット履歴を抽出して送信する
    emit requestSend(chatHistoryFromListModel(m_model));
}

/**
 * @brief AI からの応答を受信したときの処理
 */
void MainWindow::onReplyReceived(const QString &reply) {
    appendAssistantMessage(reply);
    m_lastAssistantMessage = reply;

    ui->inputField->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->inputField->setFocus();
}

/**
 * @brief エラーが発生したときの処理
 */
void MainWindow::onErrorOccurred(const QString &error) {
    appendErrorMessage("エラー: " + error);

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
 * @brief ユーザー発話をチャット表示に追加
 */
void MainWindow::appendUserMessage(const QString &message) {
    appendChatLine(m_model, ChatMessage::Role::User, message);
    ui->chatDisplay->scrollToBottom();
}

/**
 * @brief アシスタント応答をチャット表示に追加
 */
void MainWindow::appendAssistantMessage(const QString &message) {
    appendChatLine(m_model, ChatMessage::Role::Assistant, message);
    ui->chatDisplay->scrollToBottom();
}

/**
 * @brief エラー等の生メッセージをチャット表示に追加
 */
void MainWindow::appendErrorMessage(const QString &message) {
    appendRawLine(m_model, message);
    ui->chatDisplay->scrollToBottom();
}

/**
 * @brief チャットディスプレイの右クリックメニューを表示
 *
 * - 単一選択時: 編集 / 削除 を表示。アシスタント発話なら音声合成も追加
 * - 複数選択時: 削除 を表示。アシスタント発話が 1 件以上含まれる場合は
 *   音声合成も追加
 */
void MainWindow::onChatDisplayContextMenu(const QPoint &pos) {
    auto *selectionModel = ui->chatDisplay->selectionModel();
    if (!selectionModel) {
        return;
    }

    // 右クリック位置のアイテムが現在の選択に含まれていなければ、
    // そのアイテムだけを単一選択し直す（一般的な GUI の挙動に合わせる）
    QModelIndex clickedIndex = ui->chatDisplay->indexAt(pos);
    if (clickedIndex.isValid() && !selectionModel->isSelected(clickedIndex)) {
        selectionModel->select(clickedIndex, QItemSelectionModel::ClearAndSelect |
                                                 QItemSelectionModel::Rows);
        ui->chatDisplay->setCurrentIndex(clickedIndex);
    }

    QModelIndexList selected = selectionModel->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    // 選択中にアシスタント発話が 1 件でもあれば音声合成可
    bool hasAssistant = false;
    for (const QModelIndex &idx : selected) {
        if (m_model->data(idx).toString().startsWith(kAssistantPrefix)) {
            hasAssistant = true;
            break;
        }
    }

    QMenu menu(this);
    if (selected.size() == 1) {
        QAction *editAction = menu.addAction("編集");
        connect(editAction, &QAction::triggered, this,
                &MainWindow::editSelectedChatItem);
    }
    QAction *deleteAction = menu.addAction("削除");
    connect(deleteAction, &QAction::triggered, this,
            &MainWindow::deleteSelectedChatItems);
    if (hasAssistant) {
        QAction *playAction = menu.addAction("音声合成");
        connect(playAction, &QAction::triggered, this,
                &MainWindow::playSelectedChatItems);
    }

    menu.exec(ui->chatDisplay->viewport()->mapToGlobal(pos));
}

/**
 * @brief 選択中のチャットアイテムを編集する
 * 単一選択時のみ有効
 */
void MainWindow::editSelectedChatItem() {
    auto *selectionModel = ui->chatDisplay->selectionModel();
    if (!selectionModel) {
        return;
    }
    QModelIndexList selected = selectionModel->selectedIndexes();
    if (selected.size() != 1) {
        return;
    }

    QModelIndex idx = selected.first();
    QString current = m_model->data(idx).toString();
    bool ok = false;
    QString newText = QInputDialog::getMultiLineText(this, "メッセージを編集",
                                                     "メッセージ:", current, &ok);
    if (!ok) {
        return;
    }
    m_model->setData(idx, newText);
}

/**
 * @brief 選択中のチャットアイテムを削除する
 * 単一・複数選択どちらでも動作する（確認なし・即時削除）
 */
void MainWindow::deleteSelectedChatItems() {
    auto *selectionModel = ui->chatDisplay->selectionModel();
    if (!selectionModel) {
        return;
    }
    QModelIndexList selected = selectionModel->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    // 後ろから削除して、行番号がずれるのを防ぐ
    QList<int> rows;
    rows.reserve(selected.size());
    for (const QModelIndex &idx : selected) {
        rows.append(idx.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        m_model->removeRow(row);
    }
}

/**
 * @brief 選択中のアシスタント発話を音声合成する
 *
 * 選択行のうち kAssistantPrefix で始まる行のみをプレフィックスを取り除いて
 * 抽出する。1 件なら synthesizeRequested、複数件なら
 * synthesizeMultipleRequested を emit する。
 */
void MainWindow::playSelectedChatItems() {
    auto *selectionModel = ui->chatDisplay->selectionModel();
    if (!selectionModel) {
        return;
    }
    QModelIndexList selected = selectionModel->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    // 行順を保つために選択インデックスを行番号で昇順ソート
    std::sort(selected.begin(), selected.end(),
              [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() < b.row();
    });

    QStringList texts;
    texts.reserve(selected.size());
    for (const QModelIndex &idx : selected) {
        const QString text = m_model->data(idx).toString();
        if (text.startsWith(kAssistantPrefix)) {
            texts.append(text.mid(kAssistantPrefix.size()));
        }
    }

    if (texts.isEmpty()) {
        return;
    }
    if (texts.size() == 1) {
        emit synthesizeRequested(texts.first());
    } else {
        emit synthesizeMultipleRequested(texts);
    }
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
