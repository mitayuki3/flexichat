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
    explicit MainWindow(ProfileManager *profileManager,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void onReplyReceived(const QString &reply);
    void onErrorOccurred(const QString &error);
    void onApiRequestStarted();
    void onApiRequestFinished();
    void onProfileChanged(const SystemPromptProfile &profile);
    void onProfileListChanged();
    void syncTtsButtons();
    void showStatusMessage(const QString &status);
    QString getPendingTtsText() const;
    void appendTtsOutput(QString const &filePath);

signals:
    void requestSend(const QString &message);
    void profileChangeRequested(const QString &profileId);
    void openSettingsRequested();
    void synthesizeRequested(const QString &text);
    void synthesizeMultipleRequested(QStringList const &list);
    void autoplayChanged(bool checked);
    void voiceChanged(const QString &voice);
    void ttsPlayRequested();
    void ttsFileSelected(const QString &filePath);
    void ttsFileActivated(const QString &filePath);

private slots:
    void onSendClicked();
    void onProfileComboActivated(int index);
    void onPlayTtsClicked();
    void onChatDisplayClicked(const QModelIndex &index);
    void generateTtsSpeech();
    void onTtsListRowChanged(int row);
    void onTtsListActivated(const QModelIndex &index);
    void commitProfileEdits();

private:
    Ui::MainWindow *ui;
    ProfileManager *m_profileManager;
    QStringListModel *m_model;
    QString m_lastAssistantMessage;
    QString m_pendingTtsText;
    QString m_displayedProfileId;
    bool m_loadingProfileFields = false;
    bool m_committingFromEditor = false;

    void setupUI();
    void connectSignals();
    void appendMessage(const QString &role, const QString &message);
    void populateProfileCombo();
    void loadProfileIntoEditor(const SystemPromptProfile &profile);
};
