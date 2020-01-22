#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QtWidgets>
#include <QRegExp>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    QString _rscrDir;
    QTextBrowser *_txtEdit;
    QPushButton *_b2u;
    QPushButton *_u2b;
    QPushButton *_norm;
    QToolButton *_betaButton;
    QToolButton *_capsButton;
    QToolButton *_autoName;
    QString _texte;
    bool _isCSV;

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
    void ouvrir();
    void sauver(QString nomFichier = "");
    QString beta2unicode(QString f,bool beta = true);
    QString uni2betacode(QString f);

    QStringList _beta;
    QStringList _uni;

    QString repertoire;
    QString nom;

    // Pour le dialogue à propos des fichiers CSV
    QDialog * dialCSV;
    QButtonGroup * group1;
    QButtonGroup * group2;
    QRadioButton * rbAll;
    QRadioButton * rbRange;
    QLineEdit * range;
    QRadioButton * rbTab;
    QRadioButton * rbComma;
    bool _annule;

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
