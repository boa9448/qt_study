#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "meterial_theme_test_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QDir::addSearchPath("icon", ":/icon/theme");

    QFile file(":/file/dark_teal.qss");
    file.open(QFile::ReadOnly);

    QString styleSheet { file.readAll() };
    a.setStyleSheet(styleSheet);

    MainWindow w;
    w.show();
    return a.exec();
}
