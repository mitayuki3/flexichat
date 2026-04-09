#pragma once

#include "SystemPromptProfile.h"
#include <QDialog>

class ProfileManager;
class LMStudioClient;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(ProfileManager *profileManager,
                            LMStudioClient *client, QWidget *parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void onProfileSelected();
    void onAddProfile();
    void onDeleteProfile();
    void onSave();
    void onConnectionTest();

private:
    Ui::SettingsDialog *ui;
    ProfileManager *m_profileManager;
    LMStudioClient *m_client;
    QString m_currentEditingId;

    void populateProfileList();
    void loadProfileIntoFields(const QString &profileId);
    void clearFields();
    SystemPromptProfile buildProfileFromFields() const;
};
