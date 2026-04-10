#pragma once

#include <QComboBox>
#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class LMStudioClient;
class ProfileManager;
struct SystemPromptProfile;

/**
 * @brief メインウィンドウクラス
 * アプリケーションのメインウィンドウを提供し、チャット機能を提供する
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(LMStudioClient *client, ProfileManager *profileManager,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void onReplyReceived(const QString &reply);
    void onErrorOccurred(const QString &error);
    void onApiRequestStarted();
    void onApiRequestFinished();
    void onProfileChanged(const SystemPromptProfile &profile);
    void onProfileListChanged();

private slots:
    void onSendClicked();
    void onProfileComboActivated(int index);
    void openSettings();

private:
    Ui::MainWindow *ui;
    LMStudioClient *m_client;
    ProfileManager *m_profileManager;
    QStringListModel *m_model;
    QComboBox *m_profileCombo;

    void setupUI();
    void setupToolbar();
    void connectSignals();
    void appendMessage(const QString &role, const QString &message);
    void populateProfileCombo();
};
