#ifndef MAINLOGIC_H
#define MAINLOGIC_H

#include <QObject>

struct TtsSettingsData;
class OpenAITTSClient;

class MainLogic : public QObject {
    Q_OBJECT
public:
    explicit MainLogic(TtsSettingsData const &data, QObject *parent = nullptr);

signals:
    void statusOccured(QString const &message);
    void mediaSourceChanged(const QUrl &source);
    void ttsFileCreated(const QString &filePath);

public slots:
    void updateSettings(TtsSettingsData const &data);
    void synthesize(QString const &text);
    void onReplyReceived(QString const &reply);
    void onSynthesizeCompleted(const QString &filePath);
    void onTtsFileActivated(const QString &filePath);

private:
    OpenAITTSClient *m_ttsClient;
    bool m_autoPlay;
};

#endif // MAINLOGIC_H
