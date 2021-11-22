#include "Conv_mainwindow.h"
#include <QApplication>

/**
 * \file Conv_main.cpp
 * \brief main de Greek_converter
 * \author Philippe Verkerk
 * \version 1
 * \date 2018
 *
 * Greek_converter est un petit programme qui permet
 * de convertir du betacode en unicode et réciproquement.
 */

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CMainWindow w;
    w.show();

    return a.exec();
}
