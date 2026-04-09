#include "SettingsDialog.h"
#include "LMStudioClient.h"
#include "ProfileManager.h"
#include "ui_SettingsDialog.h"
#include <QListWidgetItem>
#include <QMessageBox>

SettingsDialog::SettingsDialog(ProfileManager *profileManager,
                               LMStudioClient *client, QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsDialog),
      m_profileManager(profileManager), m_client(client) {
    ui->setupUi(this);

    populateProfileList();

    // Signal connections
    connect(ui->profileListWidget, &QListWidget::currentItemChanged, this,
            [this]() { onProfileSelected(); });
    connect(ui->addProfileButton, &QPushButton::clicked, this,
            &SettingsDialog::onAddProfile);
    connect(ui->deleteProfileButton, &QPushButton::clicked, this,
            &SettingsDialog::onDeleteProfile);
    connect(ui->testConnectionButton, &QPushButton::clicked, this,
            &SettingsDialog::onConnectionTest);
}

SettingsDialog::~SettingsDialog() { delete ui; }

void SettingsDialog::populateProfileList() {
    ui->profileListWidget->clear();
    auto profiles = m_profileManager->getAllProfiles();
    QString activeId = m_profileManager->getActiveProfileId();

    for (const auto &p : profiles) {
        auto *item = new QListWidgetItem(p.displayName());
        item->setData(Qt::UserRole, p.id);
        if (p.id == activeId) {
            item->setSelected(true);
        }
        ui->profileListWidget->addItem(item);
    }

    if (ui->profileListWidget->currentItem() == nullptr &&
        ui->profileListWidget->count() > 0) {
        ui->profileListWidget->setCurrentRow(0);
    }
}

void SettingsDialog::onProfileSelected() {
    auto *current = ui->profileListWidget->currentItem();
    if (!current) {
        return;
    }
    QString id = current->data(Qt::UserRole).toString();
    m_currentEditingId = id;
    loadProfileIntoFields(id);
}

void SettingsDialog::loadProfileIntoFields(const QString &profileId) {
    auto *profile = m_profileManager->getProfileById(profileId);
    if (!profile) {
        clearFields();
        return;
    }

    ui->nameEdit->setText(profile->name);
    ui->iconEdit->setText(profile->icon);
    ui->promptEdit->setPlainText(profile->prompt);
    ui->baseUrlEdit->setText("http://localhost:1234");
    ui->temperatureSpin->setValue(profile->temperature);
    ui->maxTokensSpin->setValue(profile->maxTokens);
}

void SettingsDialog::clearFields() {
    ui->nameEdit->clear();
    ui->iconEdit->clear();
    ui->promptEdit->clear();
    ui->temperatureSpin->setValue(0.7);
    ui->maxTokensSpin->setValue(2048);
}

void SettingsDialog::onAddProfile() {
    clearFields();
    m_currentEditingId.clear();
    ui->nameEdit->setFocus();
}

void SettingsDialog::onDeleteProfile() {
    auto *current = ui->profileListWidget->currentItem();
    if (!current) {
        return;
    }

    QString id = current->data(Qt::UserRole).toString();
    auto profiles = m_profileManager->getAllProfiles();

    if (profiles.size() <= 1) {
        QMessageBox::warning(this, "警告", "最後のプロファイルは削除できません。");
        return;
    }

    auto result = QMessageBox::question(
        this, "確認",
        QString("プロファイル「%1」を削除しますか？").arg(current->text()),
        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        m_profileManager->deleteProfile(id);
        populateProfileList();
    }
}

void SettingsDialog::onSave() {
    QString name = ui->nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "警告", "プロファイル名を入力してください。");
        ui->nameEdit->setFocus();
        return;
    }

    SystemPromptProfile profile = buildProfileFromFields();
    profile.name = name;

    if (m_currentEditingId.isEmpty()) {
        m_profileManager->addProfile(profile);
    } else {
        profile.id = m_currentEditingId;
        m_profileManager->updateProfile(profile);
    }

    populateProfileList();
    m_currentEditingId = profile.id;
}

void SettingsDialog::onConnectionTest() {
    ui->testResultLabel->setText("テスト中...");
    m_client->testConnection();

    connect(
        m_client, &LMStudioClient::connectionTestResult, this,
        [this](bool success, const QString &message) {
            ui->testResultLabel->setText(success ? "\u2713 " + message
                                                 : "\u2717 " + message);
            if (!success) {
                QMessageBox::warning(this, "\u63a5\u7d9a\u30a8\u30e9\u30fc", message);
            }
        },
        Qt::SingleShotConnection);
}

SystemPromptProfile SettingsDialog::buildProfileFromFields() const {
    SystemPromptProfile p;
    p.name = ui->nameEdit->text().trimmed();
    p.icon = ui->iconEdit->text().trimmed();
    p.prompt = ui->promptEdit->toPlainText().trimmed();
    p.temperature = ui->temperatureSpin->value();
    p.maxTokens = ui->maxTokensSpin->value();
    return p;
}
