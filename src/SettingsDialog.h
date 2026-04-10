#pragma once

#include "SystemPromptProfile.h"
#include <QDialog>

class ProfileManager;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(ProfileManager *profileManager,
                            QWidget *parent = nullptr);
    ~SettingsDialog() override;

signals:
    void connectionTestRequested();

public slots:
    void onConnectionTestResult(bool success, const QString &message);

private slots:
    void onProfileSelected();
    void onAddProfile();
    void onDeleteProfile();
    void onSave();
    void onConnectionTest();

private:
    Ui::SettingsDialog *ui;
    ProfileManager *m_profileManager;
    QString m_currentEditingId;

    void populateProfileList();
    void loadProfileIntoFields(const QString &profileId);
    void clearFields();
    SystemPromptProfile buildProfileFromFields() const;
};
