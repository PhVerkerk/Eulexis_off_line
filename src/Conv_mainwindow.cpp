#include "Conv_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    repertoire = QDir::homePath();
    createW();
    connecter();

    _rscrDir = qApp->applicationDirPath() + "/ressources/";
    QFile fListe (_rscrDir + "betunicode_gr.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    fluxL.setCodec("UTF-8");
    QString ligne;
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (!ligne.contains("!"))
        {
            QStringList ecl = ligne.split("\t");
//            if (ecl[0] == "i/+") qDebug() << ecl[1] << ecl[2][0].unicode() << ecl[2];
            _beta << ecl[0];
            _uni << ecl[2];
        }
    }
    fListe.close();
}

MainWindow::~MainWindow()
{

}

/**
 * \fn void MainWindow::closeEvent(QCloseEvent *event)
 * \brief Vérifie que le travail est sauvé
 *        avant la fermeture de l'application.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
        event->accept();
}

/**
 * @brief MainWindow::createW
 *
 * Prépare la fenêtre principale.
 *
 */
void MainWindow::createW()
{
    resize(768, 532);
    setUnifiedTitleAndToolBarOnMac(true);

    _txtEdit = new QTextBrowser(this);
    _txtEdit->setOpenLinks(false);
    _txtEdit->setOpenExternalLinks(false);
    setCentralWidget(_txtEdit);
    setObjectName("Greek converter");

    actionB2U = new QAction("Beta2Uni",this);
    actionB2U->setObjectName(QStringLiteral("Beta2Uni"));
    actionU2B = new QAction("Uni2Beta",this);
    actionU2B->setObjectName(QStringLiteral("Uni2Beta"));
    actionA_propos = new QAction("À propos",this);
    actionA_propos->setObjectName(QStringLiteral("actionA_propos"));
    quitAct = new QAction(QIcon(":/res/power.svg"), tr("&Quitter"), this);
    quitAct->setStatusTip(tr("Quitter l'application"));
    quitAct->setShortcut(
        QKeySequence(tr("Ctrl+Q")));  // QKeySequence::Quit inopérant

    _b2u = new QPushButton("Betacode to Unicode",this);
    _b2u->setMinimumWidth(250);
    _b2u->setMaximumWidth(250);
    _b2u->setToolTip("Convert a file from Betacode to UTF-8");
    _u2b = new QPushButton("Unicode to Betacode",this);
    _u2b->setMinimumWidth(250);
    _u2b->setMaximumWidth(250);
    _u2b->setToolTip("Convert a file from UTF-8 to Betacode");

    _betaButton = new QToolButton(this);
    _betaButton->setText("ϐ");
    _betaButton->setCheckable(true);
    _betaButton->setChecked(true);
    _betaButton->setToolTip("Distinguer les beta");

    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QStringLiteral("menuBar"));
    menuBar->setGeometry(QRect(0, 0, 768, 22));
    menuFichier = new QMenu("Fichier",menuBar);
    menuFichier->setObjectName(QStringLiteral("menuFichier"));

    setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    addToolBar(Qt::TopToolBarArea, mainToolBar);

    mainToolBar->addWidget(_b2u);
    mainToolBar->addWidget(_u2b);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(_betaButton);
    mainToolBar->addSeparator();
    mainToolBar->addAction(quitAct);

    menuBar->addAction(menuFichier->menuAction());
    menuFichier->addAction(actionB2U);
    menuFichier->addAction(actionU2B);
    menuFichier->addSeparator();
    menuFichier->addAction(actionA_propos);
    menuFichier->addSeparator();
    menuFichier->addAction(quitAct);

    setWindowTitle(tr("Greek Converter"));
    setWindowIcon(QIcon(":/res/Eulexis.png"));
}

void MainWindow::connecter()
{

    connect(actionB2U, SIGNAL(triggered()), this, SLOT(bet2uni()));
    connect(actionU2B, SIGNAL(triggered()), this, SLOT(uni2bet()));
    connect(_b2u, SIGNAL(clicked()), this, SLOT(bet2uni()));
    connect(_u2b, SIGNAL(clicked()), this, SLOT(uni2bet()));
    connect(actionA_propos, SIGNAL(triggered()), this, SLOT(aPropos()));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

}

/**
 * @brief MainWindow::aPropos
 *
 * Affiche une fenêtre de dialogue avec les remerciements.
 *
 */
void MainWindow::aPropos()
{
    QMessageBox::about(
        this, tr("Eulexis"),
        tr("<b>Convertisseur de Betacode en Unicode</b><br/>\n"
           "inspiré d'Eulexis</i><br/>\n"
           "Licence GPL, © Philippe Verkerk, 2019 <br/><br/>\n"
           "Merci à :<ul>\n"
           "<li>Yves Ouvrard</li>\n"
           "<li>Régis Robineau</li>\n"
           "<li>Jean-Paul Woitrain</li>\n"
           "<li>Philipp Roelli</li>\n"
           "<li>André Charbonnet (alias Chaerephon)</li>\n"
           "<li>Mark de Wilde</li>\n"
           "<li>Perseus Project</li>\n"
           "<li>Equipex Biblissima</li></ul>"));

}

void MainWindow::ouvrir()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",repertoire,"Text files (*.txt;*.csv)");
    if (!nomFichier.isEmpty())
    {
        repertoire = QFileInfo (nomFichier).absolutePath ();
        _texte.clear();
        QFile fEntree (nomFichier);
        if (fEntree.open (QFile::ReadOnly | QFile::Text))
        {
            _texte = fEntree.readAll();
            _isCSV = (QFileInfo(nomFichier).suffix() == "csv");
        }
    }
}

void MainWindow::sauver(QString nomFichier)
{
    if (nomFichier.isEmpty()) nomFichier =
        QFileDialog::getSaveFileName(this, "Sauvegarder le travail en cours", repertoire, "*.txt");
    if (!nomFichier.isEmpty())
    {
        repertoire = QFileInfo (nomFichier).absolutePath ();
        if (QFileInfo(nomFichier).suffix().isEmpty())
        {
            if (_isCSV) nomFichier.append(".csv");
            else nomFichier.append(".txt");
        }
        QFile f(nomFichier);
        if (f.open(QFile::WriteOnly))
        {
            f.write(_texte.toUtf8());
            f.close();
        }
    }
}

QString MainWindow::beta2unicode(QString f, bool beta)
{
    // Transf le betacode en unicode
        // Le sigma final
    f.replace(reSigmaFinal,"ς\\1");
//    qDebug() << f;
    if (!beta) f.replace("b","β");
    else f.replace(reBetaInitial,"\\1β");
        //if (f.startsWith("b"))
        //f = "β" + f.mid(1);
//    qDebug() << beta << f;
/*    if (f[f.size()-1].isDigit() && (f[f.size()-2]=='s'))
    {
        QChar nn = f[f.size()-1];
        f.chop(2);
        f.append("ς");
        // Le sigma final
        f.append(nn);
    }*/
    for (int i=0; i<_beta.size();i++)
        f.replace(_beta[i],_uni[i]);
//    qDebug() << beta << f;
    return f;
}

QString MainWindow::uni2betacode(QString f)
{
    // Transf l'unicode en betacode
    for (int i=0; i<_beta.size();i++)
        f.replace(_uni[i],_beta[i]);
//    for (int i=0; i<f.size(); i++)
//        if (f[i].unicode() > 127) qDebug() << f[i] << f;
    return f;
}

void MainWindow::bet2uni()
{
    // Ouvrir, transformer et sauver.

    ouvrir();
    _texte = beta2unicode(_texte,_betaButton->isChecked());
    sauver();
}

void MainWindow::uni2bet()
{
    // Ouvrir, transformer et sauver.
    ouvrir();
    _texte = uni2betacode(_texte);
    sauver();
}
