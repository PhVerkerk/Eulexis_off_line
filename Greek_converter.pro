#-------------------------------------------------
#
# Project created by QtCreator 2017-03-05T15:46:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
# qtHaveModule(printsupport): QT += printsupport

TARGET = Greek_converter
TEMPLATE = app

QT += svg

CONFIG += release_binary debug

!macx:DESTDIR = bin
OBJECTS_DIR= obj/
MOC_DIR = moc/

SOURCES += src/Conv_main.cpp\
        src/Conv_mainwindow.cpp

HEADERS  += src/Conv_mainwindow.h

RESOURCES += src/Greek_converter.qrc

!macx:
{
    RC_ICONS = src/res/Eulexis.ico
    data.path = bin/ressources
    data.files =  Eulexis_data/betunicode_gr.csv
    deploy.depends += install
    INSTALLS += data
    win32|win64:
    {
        deploy.commands = windeployqt bin/Greek_converter.exe
        QMAKE_EXTRA_TARGETS += deploy
    }
    linux:
    {
        target.path = bin
        target.file = $$TARGET
        INSTALLS += target
    }
}
macx:
{
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
    ICON = src/res/Eulexis.icns
    # install into app bundle
    # à changer en ressources
    data.path = Greek_converter.app/Contents/MacOS/ressources
    data.files =  Eulexis_data/betunicode_gr.csv
    deploy.depends += install
    INSTALLS += data
    deploy.commands = macdeployqt Greek_converter.app
    QMAKE_EXTRA_TARGETS += deploy
}
