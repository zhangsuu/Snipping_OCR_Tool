#include <QApplication>
#include <QFile>
#include <QTextStream>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 全局加载应用 QSS 样式表
    QFile qssFile(":/style.qss");
    if (!qssFile.exists()) {
        qssFile.setFileName(QCoreApplication::applicationDirPath() + "/style.qss");
    }
    if (!qssFile.exists()) {
        qssFile.setFileName("style.qss");
    }
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&qssFile);
        app.setStyleSheet(stream.readAll());
        qssFile.close();
    }

    MainWindow w;
    w.show();

    return app.exec();
}
