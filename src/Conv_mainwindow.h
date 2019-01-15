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
    QToolButton *_betaButton;
    QString _texte;
    bool _isCSV;

    QAction *quitAct;
    QAction *actionB2U;
    QAction *actionU2B;
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

private slots:
    void closeEvent(QCloseEvent *event);
    void aPropos();

    void bet2uni();
    void uni2bet();

};

#endif // MAINWINDOW_H
