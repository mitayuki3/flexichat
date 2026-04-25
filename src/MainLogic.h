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

private:
    AppSettings const *m_settings;
};

#endif // MAINLOGIC_H
