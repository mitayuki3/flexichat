#include "MainLogic.h"

MainLogic::MainLogic(AppSettings const *settings, QObject *parent)
    : QObject{parent}, m_settings{settings} {}
