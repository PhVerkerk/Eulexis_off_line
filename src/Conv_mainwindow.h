#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QtWidgets>
#include <QRegExp>

/**
 * \file Conv_mainwindow.h
 * \brief Header du CMainWindow, GUI de Greek_converter
 * \author Philippe Verkerk
 * \version 1
 * \date 2018
 *
 * Greek_converter est un petit programme qui permet
 * de convertir du betacode en unicode et réciproquement.
 */

/**
 * @brief La classe CMainWindow est la GUI de Greek_converter
 *
 * Cette classe définit la fenêtre d'interface de Greek_converter.
 * Greek_converter est un petit programme qui permet
 * de convertir du betacode en unicode et réciproquement.
 * Il travaille de fichier à fichier :
 * * @c txt, tout le texte est converti.
 * * @c csv, le séparateur peut être la virgule ou la tabulation
 * et une fenêtre de dialogue permet de sélectionner les colonnes à convertir.
 */
class CMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    CMainWindow(QWidget *parent = 0);
    ~CMainWindow();

private:
    QString _rscrDir; /*!< Le chemin complet du répertoire de ressources*/
    QTextBrowser *_txtEdit; /*!< La fenêtre principale (historique)*/
    QPushButton *_b2u; /*!< Bouton pour convertir le betacode en unicode*/
    QPushButton *_u2b; /*!< Bouton pour convertir l'unicode en betacode*/
    QPushButton *_norm; /*!< Bouton pour normaliser l'unicode*/
    QToolButton *_betaButton; /*!< Bouton pour distinguer les deux bêtas*/
    QToolButton *_capsButton; /*!< Bouton pour mettre le betacode en majuscule*/
    QToolButton *_autoName; /*!< Bouton pour générer automatiquement le nom du fichier créé*/
    QString _texte; /*!< Le texte*/
    bool _isCSV; /*!< Booléen pour dire que le fichier est un CSV*/

    QAction *quitAct;
    QAction *actionB2U;
    QAction *actionU2B;
    QAction *actionNorm;
    QAction *actionA_propos;
    QAction *betaAct;

    QMenuBar *menuBar;
    QMenu *menuFichier;

    QToolBar *mainToolBar;

    const QRegExp reLettres = QRegExp("[A-Za-z]");
    const QRegExp reSigmaFinal = QRegExp("s([ ,;:\\.!?0123456789\\n\\t\"'])");
    const QRegExp reBetaInitial = QRegExp("([ ,;:\\.!?0123456789\\n\\t\"'])b");

    void createW();
    void connecter();
    bool ouvrir();
    void sauver(QString nomFichier = "");
    QString beta2unicode(QString f,bool beta = true);
    QString uni2betacode(QString f);

    QStringList _beta; /*!< Liste des caractères grecs en betacode pour la conversion en unicode*/
    QStringList _uni; /*!< Liste des caractères grecs en unicode pour la conversion en betacode*/

    QString _repertoire; /*!< Nom du répertoire de travail*/
    QString _nom; /*!< Nom du fichier de travail*/

    // Pour le dialogue à propos des fichiers CSV
    QDialog * dialCSV;
    QButtonGroup * group1;
    QButtonGroup * group2;
    QRadioButton * rbAll;
    QRadioButton * rbRange;
    QLineEdit * range;
    QRadioButton * rbTab;
    QRadioButton * rbComma;
    bool _annule; /*!< Booléen pour dire que la sélection CSV est annulée */

    QList<int> listEntiers(QString le);

private slots:
    void closeEvent(QCloseEvent *event);
    void aPropos();

    void bet2uni();
    void uni2bet();
    void normalise();

    void annuleDial();
    void fermeDial();

};

#endif // MAINWINDOW_H
