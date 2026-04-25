#pragma once

#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QUrl>

class OpenAITTSClient : public QObject {
    Q_OBJECT

public:
    explicit OpenAITTSClient(const QString &outputDir, QObject *parent = nullptr);
    ~OpenAITTSClient() override;

    void synthesize(const QString &text);
    void stop();
    void setBaseUrl(const QString &url);
    void setApiKey(const QString &key);
    void setVoice(const QString &voice);
    void setModel(const QString &model);
    void setInstructions(const QString &instructions);
    void setFormat(const QString &format);
    void setOutputDir(const QString &dir);

signals:
    void synthesizeCompleted(const QString &filePath);
    void errorOccurred(const QString &error);
    void statusChanged(const QString &status);

private slots:
    void onSynthesizeFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QString m_baseUrl;
    QString m_apiKey;
    QString m_voice;
    QString m_model;
    QString m_instructions;
    QString m_format;
    QString m_outputDir;

    QString generateFilePath(const QString &format) const;
};
