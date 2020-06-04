#-------------------------------------------------
#
# Project created by QtCreator 2017-03-05T15:46:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
qtHaveModule(printsupport): QT += printsupport

TARGET = Eulexis
TEMPLATE = app

QT += svg

CONFIG += release_binary debug

!macx:DESTDIR = bin
OBJECTS_DIR= obj/
MOC_DIR = moc/

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/lemmatiseur.cpp

HEADERS  += src/mainwindow.h \
    src/lemmatiseur.h

RESOURCES += src/Eulexis.qrc

!macx:
{
    RC_ICONS = src/res/Eulexis.ico
    data.path = bin/ressources
    data.files =  Eulexis_data/*
    deploy.depends += install
    INSTALLS += data
    deploy.commands = windeployqt bin/Eulexis.exe
    QMAKE_EXTRA_TARGETS += deploy
}
macx:
{
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
    ICON = src/res/Eulexis.icns
    # install into app bundle
    # à changer en ressources
#    data.path = Eulexis.app/Contents/MacOS/ressources
#    data.files =  Eulexis_data/*
#    deploy.depends = install_documentation
#    deploy.depends += install
#    documentation.path = Collatinus_11.app/Contents/MacOS/doc/
#    documentation.files = doc/*.html doc/*.css
#    dmg.depends = deploy
#	dmg.commands = ./MacOS/Collatinus.sh
#    INSTALLS += documentation
#    INSTALLS += data
# Le 25 mai 2020, je supprime l'installation des données.
# En effet, elle est difficilement compatible avec la mise à jour des dicos.
    deploy.commands = macdeployqt Eulexis.app
    QMAKE_EXTRA_TARGETS += deploy
}
