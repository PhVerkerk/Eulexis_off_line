#include "mainwindow.h"
#include <QApplication>

/**
 * @file main.cpp
 * @brief Main pour Eulexis
 */

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.loadLemm();

    return a.exec();
}
