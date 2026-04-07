#include "MainWindow.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FlexiChat");
    app.setApplicationVersion("1.0.0");

    // フォント設定
    app.setFont(QFont("Yu Gothic UI", 10));

    // メインウィンドウの作成と表示
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}