#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QtWidgets>
#include <QRegExp>

#include "lemmatiseur.h"

class MainWindow;

class EditLatin : public QTextEdit
{
    Q_OBJECT

   private:
    MainWindow *mainwindow;
    int _ident;

   protected:
    void mouseReleaseEvent(QMouseEvent *e);

   public:
    EditLatin(QWidget *parent);
    bool event(QEvent *event);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString bulle(QString mot);

private:
    void createW();
    void readSettings();
    QString _rscrDir;
    QString latin2greek(QString f);
    QTextBrowser *_txtEdit;
    QTextBrowser *_lemEdit;
    QLineEdit *_lineEdit;
    QLineEdit *_lineEdit2;
    QPushButton *_lemmatiser;
    QPushButton *_lemmatiser2;
    QPushButton *_avant;
    QPushButton *_apres;
    QToolButton *_beta;
//    QToolButton *_exact;
    QAction *_exact;

    QAction *quitAct;
    QAction *balaiAct;
    QAction *actionNouveau;
    QAction *actionOuvrir;
    QAction *actionSauver;
    QAction *actionA_propos;
    QAction *actionConsulter;
    QAction *deZoomAct;
    QAction *zoomAct;
    QAction *LSJ;
    QAction *Pape;
    QAction *Bailly;
    QAction *actLSJ;
    QAction *actPape;
    QAction *actBailly;
    QAction *actComInd;
    QAction *actAnalyses;
    QAction *actTrad;
    QAction *betaAct;
    QAction *consAct;
    QAction *yeux;
    QAction *lunettes;
    QAction *lemTxt;
    QAction *lemAlpha;
    QActionGroup *groupeLang;
    QAction *langAngl;
    QAction *langAlld;
    QAction *langFr;
    QAction *fenCons;
    QAction *fenLem;
    QAction *fenTxt;

    QMenuBar *menuBar;
    QMenu *menuFichier;
    QMenu *menuDicos;
    QMenu *menuFenetre;
    QMenu *menuExtra;
    QMenu *menuAide;

    QToolBar *mainToolBar;

    const QRegExp reLettres = QRegExp("[A-Za-z]");
    int lireOptions();

    Lemmat *__lemmatiseur;

    EditLatin* _texte;
    QWidget* _second;
    QWidget* _trois;
    void createSecond();
    void createTrois();
    void connecter();

    QStringList _msg;
    bool _changements;
    bool alerte(bool sauv = true);

private slots:
    void closeEvent(QCloseEvent *event);
    void effaceRes();
    void nouveau();
    void ouvrir();
    void sauver(QString nomFichier = "");
    void aPropos();
    void valider();
    void actualiser();
    void actualiser2();
    void consulter(QString f = "");
    void majA();
    void majB();
    void majC();
    void majL();
    void majP();
    void majT();
    void avance();
    void recule();
    void suivreLien(QUrl url);
    void montrer();
    void montrer3();
    void lemmatTxt();
    void lemmatAlpha();
    void lFr();
    void lAngl();
    void lAlld();

    void avCons();
    void avLem();
    void avTxt();

public slots:
    void lemmatiser(QString f = "");

};

#endif // MAINWINDOW_H
