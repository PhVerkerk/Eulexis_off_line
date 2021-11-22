#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QtWidgets>
#include <QRegExp>
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressDialog>
#include <QDateTime>

#include "lemmatiseur.h"

/**
 * \file mainwindow.h
 * \brief Header du MainWindow, GUI d'Eulexis, et d'EditLatin
 * \author Philippe Verkerk
 * \version 1.2
 * \date 2021
 */

class MainWindow;

/**
 * @brief La classe EditLatin est dérivée de
 * QTextEdit afin de pouvoir redéfinir l'action
 * connectée au survol d'un mot par la souris
 * et au clic de souris sur un mot ou après
 * sélection d'une portion de texte.
 *
 * Copiée directement de Collatinus
 */
class EditLatin : public QTextEdit
{
    Q_OBJECT

   private:
    MainWindow *mainwindow;

   protected:
    void mouseReleaseEvent(QMouseEvent *e);

   public:
    EditLatin(QWidget *parent);
    bool event(QEvent *event);
};

/**
 * @brief La classe MainWindow crée l'interface graphique d'Eulexis
 *
 * Beaucoup de variables ne sont pas commentées ici, car elles correspondent
 * à des boutons ou des items de menu ou aux actions qui leur sont associés.
 * Les noms essaient d'être explicites.
 *
 * Fortement inspirée de Collatinus
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void loadLemm();
    QString bulle(QString mot);
    QRegExp _reApostr;
    QRegExp _reWordBoundary;

private:
    QStatusBar * _statusB;
    QLabel * _blabla;
    void createW();
    void readSettings();
    QString _rscrDir; /*!< Chemin complet pour le répertoire des ressources */
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

    QAction *exportAct;
    QAction *printAct;
    QAction *quitAct;
    QAction *balaiAct;
    QAction *actionNouveau;
    QAction *actionOuvrir;
    QAction *actionSauver;
    QAction *actionTxt2csv;
    QAction *actionTextiColor;
    QAction *actionApostr;
    QAction *actionBOM;
    QAction *actionA_propos;
    QAction *actionConsulter;
    QAction *actionBackward;
    QAction *actionForward;
    QAction *deZoomAct;
    QAction *zoomAct;
    QAction *LSJ;
    QAction *Pape;
    QAction *AbrBailly;
    QAction *Bailly;
    QAction *actLSJ;
    QAction *actPape;
    QAction *actAbrBailly;
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
    QAction *chxPolice;
    QAction *findAct;
    QAction *reFindAct;
    QAction *auxAct;
    QAction *toolsRestoreAct;

    QMenuBar *menuBar;
    QMenu *menuFichier;
    QMenu *menuEdition;
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
    QString _entete; // Pour l'entête HTML avec style.
    QStringList _historique; // Pour conserver les mots consultés
    int _histItem; // Pour naviguer dans la liste précédente
    QAction *effHistAct;

    QStringList _msg;
    bool _changements;
    bool alerte(bool sauv = true);
    QString rech;
    QString repertoire;
    bool plusPetit(QString g1, QString g2);
    bool longs(QStringList sl);
    QString _apostrophes;
    int _lang;

    // Famille de variables pour le dialogue de vérification
    QDialog * dVerif;
    QLabel * dLemme;
    QLabel * dBeta;
    QLabel * dLemAv;
    QLabel * dNum;
    QLabel * dLemAp;
    QLabel * dTrAv;
    QLabel * dTrAp;
    QLabel * dNb;
//    QLabel * dOld;
    QLineEdit * dTrEn;
    QLineEdit * dTrFr;
    QLineEdit * dTrDe;
    QTextEdit * dTrB;
//    QLineEdit * dTrB;
    QLineEdit * dComment;
    QComboBox *dLemB;
//    QFile dFichier;
//    QFile dListe;
    QString dNomFic;
    QTextStream dFlux;
    void dSetUp();
    void dAffiche(QString ligne);
    bool dValide();
    QAction * actVerif;
    int opCode;
    int indice;
    int nbLem;
    int numLem;
    QRadioButton *choixEulexis; // Déclaré ici pour pouvoir lire l'état.
    QRadioButton *choixBailly;
    QRadioButton *choixRemis;
    QPushButton *nextButton; // Déclaré ici pour changer le texte.
    QPushButton *previousButton;
//    QPushButton *undoButton;
//    QPushButton *saveButton;
    QStringList lg_Old;
    QStringList lgOrig;
    QStringList lgMod;
    QStringList elements;
    QStringList lemBailly;
    QStringList trBailly;
    QString css_vert = "background-color: #A0FFA0; selection-background-color: #009000;";
    QString css_orange = "background-color: #FFC040; selection-background-color: #B06000;";
    QString css_rouge = "background-color: #FF5040; selection-background-color: #A00020;";
    QString css_gris = "background-color: #808080; selection-background-color: #808080;";
    QString css_blanc = "background-color: #FFFFFF; selection-background-color: #000080;";

private slots:
    void exportPdf();
    void imprimer();
    void closeEvent(QCloseEvent *event);
    void effaceRes();
    void nouveau();
    void ouvrir();
    void sauver(QString nomFichier = "");
    void aPropos();
    void valider();
    void actualiser();
    void actualiser2();
    void consulter(QString f = "", bool ajoute = true);
    void majA();
    void majAB(); // Pour l'abrégé du Bailly
    void majB(); // Pour le Bailly 2020
    void majC();
    void majL();
    void majP();
    void majT();
    void avance();
    void recule();
    void forward();
    void backward();
    void effaceHist();
    void suivreLien(QUrl url);
    void montrer();
    void montrer3();
    void lemmatTxt();
    void lemmatAlpha();
    void txt2csv();
    void lFr();
    void lAngl();
    void lAlld();

    void avCons();
    void avLem();
    void avTxt();
    void choixPolice();
    void chercher();
    void rechercher();
    void auxilium();

    void toolsRestore ();

    // Famille de slots pour le dialogue de vérification.
//    void dSkip(); // ne tient pas compte des changements, rend la main
//    void dValid(); // Valide les changements.
    void dNext();
    void dPrev();
    void dSave(); // Pour sauver l'ensemble des données sans changer de lemme.
    void dUndo(); // Pour annuler les modifs
    void dChgLem(int i); // Mettre à jour la traduction quand on change de lemme.
    void verifT(); // Je définis une nouvelle action, d'où un slot.

public slots:
    void lemmatiser(QString f = "");

};

#endif // MAINWINDOW_H
