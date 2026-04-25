#ifndef MAINLOGIC_H
#define MAINLOGIC_H

#include <QObject>

class AppSettings;

class MainLogic : public QObject
{
    Q_OBJECT
public:
    explicit MainLogic(AppSettings const *settings, QObject *parent = nullptr);

signals:
    void statusOccured(QString const &message);
    void mediaSourceChanged(const QUrl &source);

public slots:
    void synthesize(QString const &text);
    void onReplyReceived(QString const &reply);
    void onSynthesizeCompleted(const QString &filePath);

private:
    AppSettings const *m_settings;
};

#endif // MAINLOGIC_H
