#pragma once

#include <QComboBox>
#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ProfileManager;
struct SystemPromptProfile;

/**
 * @brief メインウィンドウクラス
 * アプリケーションのメインウィンドウを提供し、チャット機能を提供する
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ProfileManager *profileManager, QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void onReplyReceived(const QString &reply);
    void onErrorOccurred(const QString &error);
    void onApiRequestStarted();
    void onApiRequestFinished();
    void onProfileChanged(const SystemPromptProfile &profile);
    void onProfileListChanged();
    void syncTtsButtons();
    QString getPendingTtsText() const;

signals:
    void requestSend(const QString &message);
    void profileChangeRequested(const QString &profileId);
    void openSettingsRequested();
    void synthesizeRequested(const QString &text);
    void stopTtsRequested();

private slots:
    void onSendClicked();
    void onProfileComboActivated(int index);
    void onPlayTtsClicked();
    void onStopTtsClicked();
    void onChatDisplayClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    ProfileManager *m_profileManager;
    QStringListModel *m_model;
    QComboBox *m_profileCombo;
    QString m_lastAssistantMessage;
    QString m_pendingTtsText;

    void setupUI();
    void setupToolbar();
    void connectSignals();
    void appendMessage(const QString &role, const QString &message);
    void populateProfileCombo();
};
