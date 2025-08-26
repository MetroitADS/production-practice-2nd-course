#include "mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QIcon>
#include <QFile>
#include <QStyle>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QIcon appIcon;
    QStringList iconPaths = {
        "icon.png", 
        "icon.ico",          
    };
    for (const QString& path : iconPaths) {
        if (QFile::exists(path)) {
            appIcon = QIcon(path);
            if (!appIcon.isNull()) break;
        }
    }
    if (appIcon.isNull()) {
        appIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    a.setWindowIcon(appIcon);
    MainWindow w;
    w.show();
    return a.exec();
}