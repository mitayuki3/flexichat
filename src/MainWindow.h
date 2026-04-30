#pragma once

#include "ChatMessage.h"
#include <QComboBox>
#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ChatListModel;
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
    void showStatusMessage(const QString &status);
    QString getPendingTtsText() const;
    void appendTtsOutput(QString const &filePath);

signals:
    void requestSend(const ChatHistory &history);
    void profileChangeRequested(const QString &profileId);
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
    void onChatDisplayContextMenu(const QPoint &pos);
    void editSelectedChatItem();
    void deleteSelectedChatItems();
    void playSelectedChatItems();
    void generateTtsSpeech();
    void onTtsListRowChanged(int row);
    void onTtsListActivated(const QModelIndex &index);
    void scheduleProfileCommit();
    void commitProfileEdits();
    void onAddProfileClicked();
    void onTrashProfileClicked();
    void onEmptyTrashClicked();
    void onRestoreTrashedClicked();

private:
    Ui::MainWindow *ui;
    ProfileManager *m_profileManager;
    ChatListModel *m_model;
    QString m_lastAssistantMessage;
    QString m_pendingTtsText;
    QString m_displayedProfileId;
    QTimer *m_profileCommitTimer;
    bool m_loadingProfileFields = false;
    bool m_committingFromEditor = false;

    void setupUI();
    void connectSignals();
    void appendUserMessage(const QString &message);
    void appendAssistantMessage(const QString &message);
    void appendErrorMessage(const QString &message);
    void populateProfileCombo();
    void loadProfileIntoEditor(const SystemPromptProfile &profile);
    void populateTrashList();
    void updateTrashButton();
};
