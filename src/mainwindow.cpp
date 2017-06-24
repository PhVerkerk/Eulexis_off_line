#include "mainwindow.h"

/**
 * \fn EditLatin::EditLatin (QWidget *parent): QTextEdit (parent)
 * \brief Créateur de la classe EditLatin, dérivée de
 * QTextEdit afin de pouvoir redéfinir l'action
 * connectée au clic de souris sur une ligne.
 *
 * Copié de Collatinus et adapté aux besoins.
 *
 */
EditLatin::EditLatin(QWidget *parent) : QTextEdit(parent)
{
    mainwindow = qobject_cast<MainWindow *>(parent);
}

/**
 * \fn bool EditLatin::event(QEvent *event)
 * \brief Captation du survol de la souris pour
 *        afficher une bulle d'aide.
 *
 * La routine repère la position du curseur et identifie
 * la ligne sur laquelle on traine. Il demande alors
 * à son père (mainwindow) le texte à afficher
 * dans la bulle d'aide.
 *
 */
bool EditLatin::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::ToolTip:
        {
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            QPoint P = mapFromGlobal(helpEvent->globalPos());
            QTextCursor tc = cursorForPosition(P);
            tc.select(QTextCursor::WordUnderCursor);
            QString mot = tc.selectedText();
            //  || lm[i+1].startsWith("´")
            if (mot.isEmpty ())
                return QWidget::event (event);
            if (mot.endsWith("·") || mot.endsWith("·"))
                mot.chop(1);
            if (mot.endsWith("´"))
                mot.replace("´","'");
            if (mot.endsWith("’"))
                mot.replace("’","'");
            QString txtBulle = mainwindow->bulle(mot);
            if (!txtBulle.isEmpty())
            {
                txtBulle.prepend("<p style='white-space:pre'>");
                txtBulle.append("</p>");
                QRect rect(P.x()-20,P.y()-10,40,40); // Je définis un rectangle autour de la position actuelle.
                QToolTip::setFont(font());
                QToolTip::showText(helpEvent->globalPos(), txtBulle.trimmed(),
                                   this, rect);
                // La bulle disparaît si le curseur sort du rectangle.
            }
            return true;
        }
        default:
            return QTextEdit::event(event);
    }
}

/**
 * \fn void EditLatin::mouseReleaseEvent (QMouseEvent *e)
 * \brief Captation de la fin du clic de souris.
 *
 * La routine repère la ligne sur laquelle on clique.
 * Elle envoie à son père (mainwindow) le numéro de
 * cette ligne et un identifiant.
 * Elle sert à choisir le mot à considérer
 * ou à choisir la bonne lemmatisation.
 *
 */
void EditLatin::mouseReleaseEvent(QMouseEvent *e)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) cursor.select(QTextCursor::WordUnderCursor);
    QString st = cursor.selectedText();
    if (!st.isEmpty())
    {
        if (st.endsWith("·") || st.endsWith("·"))
            st.chop(1);
        st.replace("’","'");
        st.replace("´","'");
        mainwindow->lemmatiser(st);
    }
    QTextEdit::mouseReleaseEvent(e);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createW();
    createSecond();
    createTrois();
    connecter();
    _lineEdit->setFocus();
    _msg << "%1 : word not found !";
    _msg << "%1 : mot non trouvé !";
    _msg << "%1 : Wort nicht gefunden !";
    _changements = false;
    readSettings();
//    _second->show();
}

MainWindow::~MainWindow()
{

}

void MainWindow::loadLemm()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    _blabla->setText("Chargement en cours...");
    __lemmatiseur->lireData();
    _blabla->setText("");
    QApplication::restoreOverrideCursor();
}

/**
 * \fn void MainWindow::closeEvent(QCloseEvent *event)
 * \brief Vérifie que le travail est sauvé
 *        avant la fermeture de l'application.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    bool lire = true;
    // S'il y a des changements, je demande confirmation.
    if (_changements) lire = alerte();
    if (lire)
    {
        // Je quitte si on a confirmé la sortie.
        // Mais je veux sauver la config avant
        QSettings settings("Eulexis", "Eulexis");
        settings.beginGroup("fenetre");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        settings.endGroup();
        settings.beginGroup("fenetre2");
        settings.setValue("geometry2", _second->saveGeometry());
        settings.setValue("visible2", _second->isVisible());
        settings.endGroup();
        settings.beginGroup("fenetre3");
        settings.setValue("geometry3", _trois->saveGeometry());
        settings.setValue("visible3", _trois->isVisible());
        settings.endGroup();
        settings.beginGroup("dicos");
        settings.setValue("LSJ",LSJ->isChecked());
        settings.setValue("Pape",Pape->isChecked());
        settings.setValue("Bailly",Bailly->isChecked());
        settings.setValue("Allemand",langAlld->isChecked());
        settings.setValue("Anglais",langAngl->isChecked());
        settings.setValue("Français",langFr->isChecked());
        settings.endGroup();
        settings.beginGroup("options");
        // settings.setValue("police", font.family());
        settings.setValue("beta",_beta->isChecked());
        settings.setValue("exact",_exact->isChecked());
//        int pt = _txtEdit->font().pointSize();
        QFont font = _txtEdit->font();
        settings.setValue("zoom", font.pointSize());
        settings.setValue("police", font.family());
        settings.endGroup();

        _second->hide();
        _trois->hide();
        event->accept();
    }
    else
    {
        // Si on annule la sortie.
        event->ignore();
    }
}

void MainWindow::readSettings()
{
    QSettings settings("Eulexis", "Eulexis");
    // état de la fenêtre
    settings.beginGroup("fenetre");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    settings.endGroup();
    // état de la fenêtre
    settings.beginGroup("fenetre2");
    _second->restoreGeometry(settings.value("geometry2").toByteArray());
    if (settings.value("visible2").toBool())
    {
        _second->show();
        yeux->setChecked(true);
    }
    settings.endGroup();
    settings.beginGroup("fenetre3");
    _trois->restoreGeometry(settings.value("geometry3").toByteArray());
    if (settings.value("visible3").toBool())
    {
        _trois->show();
        lunettes->setChecked(true);
    }
    settings.endGroup();
    settings.beginGroup("dicos");
    if (settings.contains("LSJ"))
    {
        LSJ->setChecked(settings.value("LSJ",true).toBool());
        Pape->setChecked(settings.value("Pape",true).toBool());
        Bailly->setChecked(settings.value("Bailly",true).toBool());
        langAlld->setChecked(settings.value("Allemand",false).toBool());
        langAngl->setChecked(settings.value("Anglais",false).toBool());
        langFr->setChecked(settings.value("Français",true).toBool());
    }
    settings.endGroup();
    settings.beginGroup("options");
    _beta->setChecked(settings.value("beta",true).toBool());
    _exact->setChecked(settings.value("exact",true).toBool());
    int pt = settings.value("zoom",14).toInt();
    QString police = settings.value("police","Times New Roman").toString();
    QFont font = QFont(police,pt);
    _txtEdit->setFont(font);
    _lemEdit->setFont(font);
    _texte->setFont(font);
    settings.endGroup();

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
    _statusB = statusBar();
//    _statusB->showMessage("Démarrage en cours...");
    _blabla = new QLabel("Démarrage en cours...");
    _statusB->addPermanentWidget(_blabla);
    setUnifiedTitleAndToolBarOnMac(true);

    _txtEdit = new QTextBrowser(this);
//    _txtEdit = new QTextEdit(this);
    _txtEdit->setOpenLinks(false);
    _txtEdit->setOpenExternalLinks(false);
    setCentralWidget(_txtEdit);
    setObjectName("Eulexis");
//    setWindowTitle(tr("Eulexis"));
//    setWindowIcon(QIcon(":/res/Eulexis.png"));
//    QFont font("Courier");
//    _txtEdit->setFont(font);
//    createSecond();

    actionNouveau = new QAction(QIcon(":/res/document-new.svg"),"Nouveau",this);
    actionNouveau->setObjectName(QStringLiteral("actionNouveau"));
    actionOuvrir = new QAction(QIcon(":/res/document-open.svg"),"Ouvrir",this);
    actionOuvrir->setObjectName(QStringLiteral("actionOuvrir"));
    actionSauver = new QAction(QIcon(":/res/document-save.svg"),"Sauver",this);
    actionSauver->setObjectName(QStringLiteral("actionSauver"));
    exportAct = new QAction(QIcon(":res/buffer-export_pdf2.png"), tr("Exporter en pdf"), this);
    printAct = new QAction(QIcon(":res/print.svg"), tr("Im&primer"), this);
    actionA_propos = new QAction("À propos",this);
    actionA_propos->setObjectName(QStringLiteral("actionA_propos"));
    quitAct = new QAction(QIcon(":/res/power.svg"), tr("&Quitter"), this);
    quitAct->setStatusTip(tr("Quitter l'application"));
    actionConsulter =
        new QAction(QIcon(":/res/dicolitt.svg"), tr("Consulter"), this);

    fenCons = new QAction(QIcon(":/res/dicolitt.svg"), tr("Fenêtre de consultation"), this);
    fenLem = new QAction(QIcon(":/res/syntaxe.svg"), tr("Fenêtre de lemmatisation"), this);
    fenTxt = new QAction(QIcon(":/res/dicolem.svg"), tr("Fenêtre de texte"), this);
    balaiAct = new QAction(QIcon(":res/edit-clear.svg"),
                           tr("&Effacer les résultats"), this);
    zoomAct = new QAction(QIcon(":res/zoom.svg"), tr("Plus grand"), this);
    deZoomAct = new QAction(QIcon(":res/dezoom.svg"), tr("Plus petit"), this);
    yeux = new QAction(QIcon(":res/occhi-03.svg"), tr("Afficher le texte"), this);
    yeux->setCheckable(true);
    yeux->setChecked(false);
    lunettes = new QAction(QIcon(":res/Eye-Glasses.svg"), tr("Afficher les lemmatisations"), this);
    lunettes->setCheckable(true);
    lunettes->setChecked(false);
    lemAlpha = new QAction(QIcon(":res/edit-alpha.svg"),tr("lemmatiser le texte"),this);
    lemAlpha->setToolTip(tr("Lemmatisation alphabétique"));
    lemTxt = new QAction(QIcon(":res/gear.svg"),tr("lemmatiser le texte"),this);
    chxPolice = new QAction(QIcon(":res/font-noun.png"), tr("Choisir la police"), this);
    findAct = new QAction(QIcon(":res/edit-find.svg"), tr("Chercher"), this);
    reFindAct = new QAction(QIcon(":res/edit-find.svg"), tr("Chercher encore"), this);

    // Les dicos
    LSJ = new QAction("Le LSJ",this);
    LSJ->setCheckable(true);
    LSJ->setChecked(true);
    Pape = new QAction("Le Pape",this);
    Pape->setCheckable(true);
    Pape->setChecked(true);
    Bailly = new QAction("Le Bailly",this);
    Bailly->setCheckable(true);
    Bailly->setChecked(true);
    consAct = new QAction("Consulter les dicos",this);
    consAct->setCheckable(true);
    consAct->setChecked(true);
    groupeLang = new QActionGroup(this);
    langAlld = new QAction("Deutsch",this);
    langAlld->setCheckable(true);
    groupeLang->addAction(langAlld);
    langAngl = new QAction("English",this);
    langAngl->setCheckable(true);
    groupeLang->addAction(langAngl);
    langFr = new QAction("Français",this);
    langFr->setCheckable(true);
    groupeLang->addAction(langFr);
    langFr->setChecked(true);

    actLSJ = new QAction("Mettre à jour le LSJ",this);
    actPape = new QAction("Mettre à jour le Pape",this);
    actBailly = new QAction("Mettre à jour le Bailly",this);
    actAnalyses = new QAction("Mettre à jour les analyses",this);
    actTrad = new QAction("Mettre à jour les traductions",this);
    actComInd = new QAction("Mettre à jour l'index commun",this);

    // raccourcis
    zoomAct->setShortcut(QKeySequence::ZoomIn);
    deZoomAct->setShortcut(QKeySequence::ZoomOut);
    findAct->setShortcut(QKeySequence::Find);
    reFindAct->setShortcut(QKeySequence::FindNext);
    actionNouveau->setShortcuts(QKeySequence::New);
    actionOuvrir->setShortcuts(QKeySequence::Open);
    actionSauver->setShortcuts(QKeySequence::Save);
    printAct->setShortcuts(QKeySequence::Print);
    quitAct->setShortcut(
        QKeySequence(tr("Ctrl+Q")));  // QKeySequence::Quit inopérant
    balaiAct->setShortcut(
        QKeySequence(tr("Ctrl+D")));  // Ctrl+D pour effacer

    _lineEdit = new QLineEdit("",this);
    _lineEdit->setMinimumWidth(100);
    _lineEdit->setMaximumWidth(100);
    _lineEdit2 = new QLineEdit("",this);
    _lineEdit2->setMinimumWidth(100);
    _lineEdit2->setMaximumWidth(100);
    _lemmatiser = new QPushButton("Lemmatiser",this);
    _lemmatiser->setMinimumWidth(100);
    _lemmatiser->setMaximumWidth(100);
    _lemmatiser->setToolTip("Lemmatiser");
    _lemmatiser2 = new QPushButton("Lemmatiser",this);
    _lemmatiser2->setMinimumWidth(100);
    _lemmatiser2->setMaximumWidth(100);
    _lemmatiser2->setToolTip("Lemmatiser");
    _avant = new QPushButton("Avant",this);
    _avant->setMinimumWidth(100);
    _avant->setMaximumWidth(100);
    _avant->setToolTip("Consulter mot avant");
    _apres = new QPushButton("Apres",this);
    _apres->setMinimumWidth(100);
    _apres->setMaximumWidth(100);
    _apres->setToolTip("Consulter mot après");
    _beta = new QToolButton(this);
    _beta->setText("ϐ");
    _beta->setCheckable(true);
    _beta->setChecked(true);
    _beta->setToolTip("Distinguer les beta");
//    _exact = new QToolButton(this);
//    _exact->setText("=");
    _exact = new QAction(QIcon(":res/egal.png"), tr("Forme exacte"), this);
    _exact->setCheckable(true);
    _exact->setChecked(true);
    _exact->setToolTip("Forme exacte");

//    QToolButton *tbDicLitt = new QToolButton(this);
//    tbDicLitt->setDefaultAction(actionConsulter);



    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QStringLiteral("menuBar"));
    menuBar->setGeometry(QRect(0, 0, 768, 22));
    menuFichier = new QMenu("Fichier",menuBar);
    menuFichier->setObjectName(QStringLiteral("menuFichier"));
    menuEdition = new QMenu("Edition",menuBar);
    menuEdition->setObjectName(QStringLiteral("menuEdition"));
    menuDicos = new QMenu("Dicos",menuBar);
    menuFenetre = new QMenu("Fenêtres",menuBar);
    menuExtra = new QMenu("Extra",menuBar);
    menuExtra->setObjectName(QStringLiteral("menuExtra"));
    menuAide = new QMenu("Options",menuBar);
    menuAide->setObjectName(QStringLiteral("menuAide"));
    setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    addToolBar(Qt::TopToolBarArea, mainToolBar);
/*    mainToolBar->addAction(actionNouveau);
    mainToolBar->addAction(actionOuvrir);
    mainToolBar->addAction(actionSauver);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionDebut);
    mainToolBar->addAction(actionPrecedente);
    mainToolBar->addWidget(editNumPhr);
    mainToolBar->addAction(actionSuivante);
    mainToolBar->addAction(actionFin);
    mainToolBar->addAction(findAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionFusion);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionCouleurs);
    mainToolBar->addAction(zoomAct);
    mainToolBar->addAction(deZoomAct);
    */
    mainToolBar->addWidget(_lineEdit);
    mainToolBar->addWidget(_lemmatiser);
//    mainToolBar->addWidget(_beta);
//    mainToolBar->addWidget(_exact);
    mainToolBar->addSeparator();
// Ajouter ici ce qu'il faut pour les dicos
    mainToolBar->addAction(actionConsulter);
    mainToolBar->addWidget(_avant);
    mainToolBar->addWidget(_apres);
    mainToolBar->addSeparator();
    mainToolBar->addAction(yeux);
    mainToolBar->addAction(lunettes);
    mainToolBar->addSeparator();
    mainToolBar->addAction(quitAct);

    menuBar->addAction(menuFichier->menuAction());
    menuBar->addAction(menuEdition->menuAction());
    menuBar->addAction(menuDicos->menuAction());
    menuBar->addAction(menuFenetre->menuAction());
    menuBar->addAction(menuExtra->menuAction());
    menuBar->addAction(menuAide->menuAction());
    menuFichier->addAction(actionNouveau);
    menuFichier->addAction(actionOuvrir);
    menuFichier->addAction(actionSauver);
    menuFichier->addSeparator();
    menuFichier->addAction(exportAct);
    menuFichier->addAction(printAct);
/*    menuFichier->addSeparator();
    menuFichier->addAction(yeux);
    menuFichier->addAction(lunettes);
    menuFichier->addSeparator();
    menuFichier->addAction(balaiAct);*/
    menuFichier->addSeparator();
    menuFichier->addAction(quitAct);

    menuEdition->addAction(findAct);
    menuEdition->addAction(reFindAct);
    menuEdition->addSeparator();
    menuEdition->addAction(zoomAct);
    menuEdition->addAction(deZoomAct);
    menuEdition->addSeparator();
    menuEdition->addAction(chxPolice);
    menuEdition->addSeparator();
    menuEdition->addAction(balaiAct);

//    menuDicos->addAction(consAct);
//    menuDicos->addSeparator();
    menuDicos->addAction(LSJ);
    menuDicos->addAction(Pape);
    menuDicos->addAction(Bailly);
    menuDicos->addSeparator();
    menuDicos->addAction(langAlld);
    menuDicos->addAction(langAngl);
    menuDicos->addAction(langFr);
//    menuDicos->addSeparator();
//    menuDicos->addAction(balaiAct);

    menuFenetre->addAction(yeux);
    menuFenetre->addAction(lunettes);
    menuFenetre->addSeparator();
    menuFenetre->addAction(fenCons);
    menuFenetre->addAction(fenLem);
    menuFenetre->addAction(fenTxt);

    menuExtra->addAction(actAnalyses);
    menuExtra->addAction(actTrad);
    menuExtra->addSeparator();
    menuExtra->addAction(actLSJ);
    menuExtra->addAction(actPape);
    menuExtra->addAction(actBailly);
    menuExtra->addSeparator();
    menuExtra->addAction(actComInd);
    menuAide->addAction(actionA_propos);

    setWindowTitle(tr("Eulexis"));
    setWindowIcon(QIcon(":/res/Eulexis.png"));

    _rscrDir = qApp->applicationDirPath() + "/ressources/";
    __lemmatiseur = new Lemmat(_rscrDir);
}

void MainWindow::connecter()
{

    connect(actionNouveau, SIGNAL(triggered()), this, SLOT(nouveau()));
    connect(actionOuvrir, SIGNAL(triggered()), this, SLOT(ouvrir()));
    connect(actionSauver, SIGNAL(triggered()), this, SLOT(sauver()));
    connect(exportAct, SIGNAL(triggered()), this, SLOT(exportPdf()));
    connect(printAct, SIGNAL(triggered()), this, SLOT(imprimer()));
    connect(actionA_propos, SIGNAL(triggered()), this, SLOT(aPropos()));
    connect(yeux, SIGNAL(triggered()), this, SLOT(montrer()));
    connect(lunettes, SIGNAL(triggered()), this, SLOT(montrer3()));
    connect(balaiAct, SIGNAL(triggered()), this, SLOT(effaceRes()));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

    connect(fenCons, SIGNAL(triggered()), this, SLOT(avCons()));
    connect(fenLem, SIGNAL(triggered()), this, SLOT(avLem()));
    connect(fenTxt, SIGNAL(triggered()), this, SLOT(avTxt()));
    connect(chxPolice, SIGNAL(triggered()), this, SLOT(choixPolice()));

    connect(zoomAct, SIGNAL(triggered()), _txtEdit, SLOT(zoomIn()));
    connect(deZoomAct, SIGNAL(triggered()), _txtEdit, SLOT(zoomOut()));
    connect(zoomAct, SIGNAL(triggered()), _lemEdit, SLOT(zoomIn()));
    connect(deZoomAct, SIGNAL(triggered()), _lemEdit, SLOT(zoomOut()));
    connect(zoomAct, SIGNAL(triggered()), _texte, SLOT(zoomIn()));
    connect(deZoomAct, SIGNAL(triggered()), _texte, SLOT(zoomOut()));

    connect(_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(actualiser()));
    connect(_lineEdit2, SIGNAL(textChanged(QString)), this, SLOT(actualiser2()));
//    connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(valider()));
    connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(consulter()));
    connect(_lineEdit2, SIGNAL(returnPressed()), this, SLOT(lemmatiser()));
    connect(_lemmatiser, SIGNAL(pressed()), this, SLOT(lemmatiser()));
    connect(_lemmatiser2, SIGNAL(pressed()), this, SLOT(lemmatiser()));
    connect(actionConsulter, SIGNAL(triggered()), this, SLOT(consulter()));
    connect(_avant, SIGNAL(clicked()), this, SLOT(avance()));
    connect(_apres, SIGNAL(clicked()), this, SLOT(recule()));

    connect(actAnalyses, SIGNAL(triggered()), this, SLOT(majA()));
    connect(actTrad, SIGNAL(triggered()), this, SLOT(majT()));
    connect(actBailly, SIGNAL(triggered()), this, SLOT(majB()));
    connect(actComInd, SIGNAL(triggered()), this, SLOT(majC()));
    connect(actLSJ, SIGNAL(triggered()), this, SLOT(majL()));
    connect(actPape, SIGNAL(triggered()), this, SLOT(majP()));
    connect(_txtEdit, SIGNAL(anchorClicked(QUrl)),this, SLOT(suivreLien(QUrl)));
    connect(_lemEdit, SIGNAL(anchorClicked(QUrl)),this, SLOT(suivreLien(QUrl)));

    connect(langAlld, SIGNAL(triggered()), this, SLOT(lAlld()));
    connect(langAngl, SIGNAL(triggered()), this, SLOT(lAngl()));
    connect(langFr, SIGNAL(triggered()), this, SLOT(lFr()));

    connect(lemAlpha, SIGNAL(triggered()),this, SLOT(lemmatAlpha()));
    connect(lemTxt, SIGNAL(triggered()),this, SLOT(lemmatTxt()));

    connect(findAct, SIGNAL(triggered()), this, SLOT(chercher()));
    connect(reFindAct, SIGNAL(triggered()), this, SLOT(rechercher()));

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
        tr("<b>Eulexis</b><br/>\n"
           "<i>Lemmatiseur de Grec ancien<br/>\n inspiré de Collatinus</i><br/>\n"
           "Licence GPL, © Philippe Verkerk, 2017 <br/><br/>\n"
           "Merci à :<ul>\n"
           "<li>Yves Ouvrard</li>\n"
           "<li>Jean-Paul Woitrain</li>\n"
           "<li>Philipp Roelli</li>\n"
           "<li>Chaerephon</li>\n"
           "<li>Perseus Project</li>\n"
           "<li>Equipex Biblissima</li></ul>"));

}

void MainWindow::majA()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",QDir::homePath(),"Text files (*.txt)");
    if (nomFichier == _rscrDir + "analyses_gr.txt")
    {
        // C'est interdit !
        QMessageBox::about(
            this, tr("Attention !"),
            tr("Il est impossible de mettre à jour le fichier de travail.\n"
               "Il faut le copier <b>ailleurs</b> pour le modifier.\n"
               "C'est ensuite le fichier modifié qu'il faudra importer.\n"));
        return;
    }

    if (!nomFichier.isEmpty())
    {
        if (QFile::exists(_rscrDir + "analyses_gr.txt"))
        {
            QFile::remove(_rscrDir + "analyses_gr.bak");
            QFile::rename(_rscrDir + "analyses_gr.txt",_rscrDir + "analyses_gr.bak");
        }
        // Si le fichier existait, je le renomme en .bak
        if (QFile::exists(_rscrDir + "analyses_gr.txt"))
            QFile::remove(_rscrDir + "analyses_gr.txt");
        QFile::copy(nomFichier,_rscrDir + "analyses_gr.txt");
        // Je copie le fichier dans les ressources.
        __lemmatiseur->lireAnalyses();
//        __lemmatiseur->majAnalyses(nomFichier);
    }
}

void MainWindow::majT()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",QDir::homePath(),"CSV files (*.csv)");
    if (nomFichier == _rscrDir + "trad_gr_en_fr_de.csv")
    {
        // C'est interdit !
        QMessageBox::about(
            this, tr("Attention !"),
            tr("Il est impossible de mettre à jour le fichier de travail.\n"
               "Il faut le copier <b>ailleurs</b> pour le modifier.\n"
               "C'est ensuite le fichier modifié qu'il faudra importer.\n"));
        return;
    }

    if (!nomFichier.isEmpty())
    {
        if (QFile::exists(_rscrDir + "trad_gr_en_fr_de.csv"))
        {
            QFile::remove(_rscrDir + "trad_gr_en_fr_de.bak");
            QFile::rename(_rscrDir + "trad_gr_en_fr_de.csv",_rscrDir + "trad_gr_en_fr_de.bak");
        }
        // Si le fichier existait, je le renomme en .bak
        if (QFile::exists(_rscrDir + "trad_gr_en_fr_de.csv"))
            QFile::remove(_rscrDir + "trad_gr_en_fr_de.csv");
        QFile::copy(nomFichier,_rscrDir + "trad_gr_en_fr_de.csv");
        // Je copie le fichier dans les ressources.
        __lemmatiseur->lireTraductions();
    }
}

void MainWindow::majL()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier du LSJ",QDir::homePath(),"Text files (*.txt)");
    if (!nomFichier.isEmpty())
    {
        QString nomCourt = nomFichier.section("/",-1);

        if (nomFichier == _rscrDir + nomCourt)
        {
            // C'est interdit !
            QMessageBox::about(
                this, tr("Attention !"),
                tr("Il est impossible de mettre à jour le fichier de travail.\n"
                   "Il faut le copier <b>ailleurs</b> pour le modifier.\n"
                   "C'est ensuite le fichier modifié qu'il faudra importer.\n"));
            return;
        }

        if (QFile::exists(_rscrDir + "LSJ.csv"))
        {
            QFile::remove(_rscrDir + "LSJ.bak");
            QFile::rename(_rscrDir + "LSJ.csv",_rscrDir + "LSJ.bak");
        }
        // Si l'index existait, je le renomme en .bak
        if (QFile::exists(_rscrDir + nomCourt))
            QFile::remove(_rscrDir + nomCourt);
        QFile::copy(nomFichier,_rscrDir + nomCourt);
        // Je copie le fichier dans les ressources.

        __lemmatiseur->majLSJ(nomCourt);
    }
}

void MainWindow::majB()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier du Bailly",QDir::homePath(),"Text files (*.txt)");
    if (!nomFichier.isEmpty())
    {
        QString nomCourt = nomFichier.section("/",-1);

        if (nomFichier == _rscrDir + nomCourt)
        {
            // C'est interdit !
            QMessageBox::about(
                this, tr("Attention !"),
                tr("Il est impossible de mettre à jour le fichier de travail.\n"
                   "Il faut le copier <b>ailleurs</b> pour le modifier.\n"
                   "C'est ensuite le fichier modifié qu'il faudra importer.\n"));
            return;
        }

        if (QFile::exists(_rscrDir + "Bailly.csv"))
        {
            QFile::remove(_rscrDir + "Bailly.bak");
            QFile::rename(_rscrDir + "Bailly.csv",_rscrDir + "Bailly.bak");
        }
        // Si l'index existait, je le renomme en .bak
        if (QFile::exists(_rscrDir + nomCourt))
            QFile::remove(_rscrDir + nomCourt);
        QFile::copy(nomFichier,_rscrDir + nomCourt);
        // Je copie le fichier dans les ressources.

        __lemmatiseur->majBailly(nomCourt);
    }
}

void MainWindow::majP()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier du Pape",QDir::homePath(),"Text files (*.txt)");
    if (!nomFichier.isEmpty())
    {
        QString nomCourt = nomFichier.section("/",-1);

        if (nomFichier == _rscrDir + nomCourt)
        {
            // C'est interdit !
            QMessageBox::about(
                this, tr("Attention !"),
                tr("Il est impossible de mettre à jour le fichier de travail.\n"
                   "Il faut le copier <b>ailleurs</b> pour le modifier.\n"
                   "C'est ensuite le fichier modifié qu'il faudra importer.\n"));
            return;
        }

        if (QFile::exists(_rscrDir + "Pape.csv"))
        {
            QFile::remove(_rscrDir + "Pape.bak");
            QFile::rename(_rscrDir + "Pape.csv",_rscrDir + "Pape.bak");
        }
        // Si l'index existait, je le renomme en .bak
        if (QFile::exists(_rscrDir + nomCourt))
            QFile::remove(_rscrDir + nomCourt);
        QFile::copy(nomFichier,_rscrDir + nomCourt);
        // Je copie le fichier dans les ressources.

        __lemmatiseur->majPape(nomCourt);
    }
}

void MainWindow::majC()
{
    __lemmatiseur->indexCommun();
}

void MainWindow::nouveau()
{
    _texte->clear();
    if (!_second->isVisible())
    {
        _second->show();
        yeux->setChecked(true);
    }
}

void MainWindow::ouvrir()
{
    nouveau();
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",QDir::homePath(),"Text files (*.txt)");
    if (!nomFichier.isEmpty())
    {
        QString txt;
        QFile fEntree (nomFichier);
        if (fEntree.open (QFile::ReadOnly | QFile::Text))
        {
            txt = fEntree.readAll();
        }
        if (txt.contains("<--- Eulexis !?./ --->"))
        {
            bool lire = true;
            // S'il y a des changements, je demande confirmation.
            if (_changements) lire = alerte(false);
            // Ici, le bouton par défaut est "Ne pas sauver".
            if (lire)
            {
                // Le fichier a été sauvé par Eulexis pour restaurer une session de travail
                int i = txt.indexOf("<--- Eulexis !?./ --->");
                _texte->setText(txt.mid(0,i));
                txt = txt.mid(i + 23);
                _lemEdit->setHtml(txt);
                _changements = false;
                // Je viens de rétablir un texte déjà sauvé.
            }
        }
        else _texte->setText(txt);
    }
}

void MainWindow::sauver(QString nomFichier)
{
    if (nomFichier.isEmpty()) nomFichier =
        QFileDialog::getSaveFileName(this, "Sauvegarder le travail en cours", QDir::homePath(), "*.txt");
    if (!nomFichier.isEmpty())
    {
        _changements = false;
        if (QFileInfo(nomFichier).suffix().isEmpty()) nomFichier.append(".txt");
        QFile f(nomFichier);
        if (f.open(QFile::WriteOnly))
        {
            QString txt = _texte->toPlainText();
            f.write(txt.toUtf8());
            txt = "\n<--- Eulexis !?./ --->\n";
            f.write(txt.toUtf8());
            txt = _lemEdit->toHtml();
            f.write(txt.toUtf8());
            f.close();
        }
    }
}

void MainWindow::avance()
{
    consulter(_avant->text());
}

void MainWindow::recule()
{
    consulter(_apres->text());
}

void MainWindow::consulter(QString f)
{
    if (f.isEmpty()) f = _lineEdit->text().toLower();
    if (f.isEmpty()) return;
    _txtEdit->clear();
    QStringList donnees;
    QString av = "";
    QString ap = "";
    QString liens = "";
    if (LSJ->isChecked())
    {
        QStringList LSJdata = __lemmatiseur->consLSJ(f);
        if (!LSJdata.isEmpty())
        {
            av = LSJdata[0];
            LSJdata.removeFirst();
            ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" LSJ " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#lsj'>LSJ 1940 : </a></h3>\n");
            LSJdata[0].prepend("<h3><a name='#lsj'>LSJ 1940 : </a></h3>\n");
            donnees << LSJdata;
        }
    }
    if (Pape->isChecked())
    {
        QStringList LSJdata = __lemmatiseur->consPape(f);
        if (!LSJdata.isEmpty())
        {
            if ((av < LSJdata[0]) || (av == "")) av = LSJdata[0];
            LSJdata.removeFirst();
            if ((ap > LSJdata[0]) || (ap == "")) ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" Pape " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#Pape'>Pape 1880 : </a></h3>\n");
            LSJdata[0].prepend("<h3><a name='#Pape'>Pape 1880 : </a></h3>\n");
            donnees << LSJdata;
        }
    }
    if (Bailly->isChecked())
    {
        QStringList LSJdata = __lemmatiseur->consBailly(f);
        if (!LSJdata.isEmpty())
        {
            if ((av < LSJdata[0]) || (av == "")) av = LSJdata[0];
            LSJdata.removeFirst();
            if ((ap > LSJdata[0]) || (ap == "")) ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" Bailly " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#Bailly'>Bailly abr. 1919 : </a></h3>\n");
            LSJdata[0].prepend("<h3><a name='#Bailly'>Bailly abr. 1919 : </a></h3>\n");
            donnees << LSJdata;
        }
    }
    if (donnees.isEmpty())
    {
        if (f.contains(reLettres)) f = __lemmatiseur->beta2unicode(f);
        _txtEdit->setHtml(_msg[__lemmatiseur->cible()].arg(f));
    }
    else
    {
        liens.append("\n<br />");
        liens.prepend("<br />\n");
        _avant->setText(av);
        _apres->setText(ap);
        _txtEdit->setHtml(liens + donnees.join(liens));
    }
}

QString MainWindow::bulle(QString mot)
{
    QStringList llem = __lemmatiseur->lemmatise(mot,_beta->isChecked());
    // Si j'ai passé une forme en grec avec décorations,
    // la première peut être en rouge si elle est exacte.
    QString res = "";
    if (llem.isEmpty()) res = "Mot inconnu !";
    else
    {
        res = "<ul>\n<li>";
        if (_exact->isChecked() && llem[0].startsWith("<span"))
            res.append(llem[0]);
        else res.append(llem.join("</li>\n<li>"));
        res.append("</li></ul>\n<br/>");
    }
    return res;
}

void MainWindow::lemmatiser(QString f)
{
    _changements = true;
    bool vide = f.isEmpty();
    if (vide) f = _lineEdit->text();
    // Si on n'a pas passé de mot, je prends le contenu de _lineEdit.
    if (f.isEmpty()) return;
    if (!_trois->isVisible())
    {
        _trois->show();
        lunettes->setChecked(true);
    }
    QStringList llem = __lemmatiseur->lemmatise(f,_beta->isChecked());
    // Si j'ai passé une forme en grec avec décorations,
    // la première peut être en rouge si elle est exacte.
    QString res = "<h4>";
    if (vide) res.append(_lemmatiser->text());
    else res.append(f);
    if (llem.isEmpty()) res.append("</h4> : Not found !\n<br/>");
    else
    {
        res.append("</h4><ul>\n<li>");
        if (_exact->isChecked() && llem[0].startsWith("<span"))
            res.append(llem[0]);
        else res.append(llem.join("</li>\n<li>"));
        res.append("</li></ul>\n<br/>");
    }
    QString texteHtml = _lemEdit->toHtml();
    texteHtml.insert(texteHtml.indexOf("</body>"),res);
    _lemEdit->setText(texteHtml);
    _lemEdit->moveCursor(QTextCursor::End);
}

void MainWindow::actualiser()
{
    QString lin = _lineEdit->text();
    _lineEdit2->setText(lin);
    if (lin.contains(reLettres))
        lin = latin2greek(lin);
    if (!lin.isEmpty())
    {
        _lemmatiser->setText(lin);
        _lemmatiser2->setText(lin);
        actionConsulter->setToolTip("Consulter l'article " + lin);
    }
}

void MainWindow::actualiser2()
{
    QString lin = _lineEdit2->text();
    _lineEdit->setText(lin);
    if (lin.contains(reLettres))
        lin = latin2greek(lin);
    if (!lin.isEmpty())
    {
        _lemmatiser->setText(lin);
        _lemmatiser2->setText(lin);
        actionConsulter->setToolTip("Consulter l'article " + lin);
    }
}

QString MainWindow::latin2greek(QString f)
{
    if (f.endsWith("s"))
    {
        f.chop(1);
        f.append("ς");
        // Le sigma final
    }
    if (_beta->isChecked())
    {
        if (f.startsWith("b"))
            f = "β" + f.mid(1);
        f.replace("b","ϐ");
    }
    else f.replace("b","β");
    f.replace("a","α");
    f.replace("g","γ");
    f.replace("d","δ");
    f.replace("e","ε");
    f.replace("z","ζ");
    f.replace("h","η");
    f.replace("q","θ");
    f.replace("i","ι");
    f.replace("k","κ");
    f.replace("l","λ");
    f.replace("m","μ");
    f.replace("n","ν");
    f.replace("c","ξ");
    f.replace("o","ο");
    f.replace("p","π");
    f.replace("r","ρ");
    f.replace("s","σ");
    f.replace("t","τ");
    f.replace("u","υ");
    f.replace("f","φ");
    f.replace("x","χ");
    f.replace("y","ψ");
    f.replace("w","ω");
    f.replace("A","Α");
    f.replace("B","Β");
    f.replace("G","Γ");
    f.replace("D","Δ");
    f.replace("E","Ε");
    f.replace("Z","Ζ");
    f.replace("H","Η");
    f.replace("Q","Θ");
    f.replace("I","Ι");
    f.replace("K","Κ");
    f.replace("L","Λ");
    f.replace("M","Μ");
    f.replace("N","Ν");
    f.replace("C","Ξ");
    f.replace("O","Ο");
    f.replace("P","Π");
    f.replace("R","Ρ");
    f.replace("S","Σ");
    f.replace("T","Τ");
    f.replace("U","Υ");
    f.replace("F","Φ");
    f.replace("X","Χ");
    f.replace("Y","Ψ");
    f.replace("W","Ω");
    f.replace("v","ϝ"); // digamma
    f.replace("V","Ϝ");
    return f;
}

int MainWindow::lireOptions()
{
    int opt=0;
    if (LSJ->isChecked()) opt++;
    if (Pape->isChecked()) opt += 2;
    if (Bailly->isChecked()) opt += 4;
    return opt;
}

void MainWindow::effaceRes()
{
    bool lire = true;
    // S'il y a des changements, je demande confirmation.
    if (_changements) lire = alerte(false);
    // Ici, le bouton par défaut est "Ne pas sauver".
    if (lire)
    {
        _changements = false;
        _lemEdit->clear();
    }
}

void MainWindow::suivreLien(QUrl url)
{
    QString lien = url.toString();
    if (lien.contains("§")) _txtEdit->scrollToAnchor("#" + lien.section("§",-1));
    else if (lien.contains("@"))
    {
        lien.replace("≤","<");
        lien.replace("≥",">");
        QToolTip::showText(_txtEdit->cursor().pos(), lien.mid(1).trimmed(), this);
    }

    else
    {
        lien = lien.mid(13);
        lien.replace("%7C","|");
        qDebug() << lien << __lemmatiseur->beta2unicode(lien,false);
        consulter(__lemmatiseur->beta2unicode(lien,false));
    }
    // Normalement, c'est un # pour aller à une ancre.
    // Pour résoudre le conflit avec les "#1" etc... utilisés en betacode,
    // j'ai mis des "§" dans les liens pour faire les renvois.
}

void MainWindow::valider()
{
//    if (consAct->isChecked()) consulter();
//    else lemmatiser();
}

/**
 * @brief MainWindow::createSecond
 *
 * Création d'un EditLatin pour afficher le texte.
 *
 */
void MainWindow::createSecond()
{
    _second = new QWidget();
    _second->setObjectName("Fenêtre de texte");
    _second->resize(600,500);
//    _second->setContentsMargins(0,0,0,0);
    QToolBar *secTool = new QToolBar;
    secTool->addAction(actionNouveau);
    secTool->addAction(actionOuvrir);
    secTool->addAction(actionSauver);
    secTool->addSeparator();
    secTool->addAction(_exact);
    secTool->addSeparator();
    secTool->addAction(lemAlpha);
    secTool->addAction(lemTxt);
    secTool->addSeparator();
    secTool->addAction(yeux);
    secTool->addAction(lunettes);
    secTool->addAction(balaiAct);
    secTool->addSeparator();
    secTool->addAction(quitAct);
    QVBoxLayout *vLayout = new QVBoxLayout(_second);
    vLayout->setSpacing(0);
    vLayout->setMargin(0);
    _texte = new EditLatin(this);
    vLayout->addWidget(secTool);
    vLayout->addWidget(_texte);

    _second->setWindowTitle("Fenêtre de texte pour Eulexis");
    _second->setWindowIcon(QIcon(":/res/Eulexis.png"));
}

void MainWindow::createTrois()
{
    _trois = new QWidget();
    _trois->setObjectName("Fenêtre de lemmatisation");
    _trois->resize(550,500);
//    _second->setContentsMargins(0,0,0,0);
    QToolBar *secTool = new QToolBar;
/*    secTool->addAction(actionNouveau);
    secTool->addAction(actionOuvrir);
    secTool->addAction(actionSauver);
    secTool->addSeparator();
    secTool->addAction(lemAlpha);
    secTool->addAction(lemTxt);*/
    secTool->addWidget(_lineEdit2);
    secTool->addWidget(_lemmatiser2);
    secTool->addWidget(_beta);
//    secTool->addWidget(_exact);
    secTool->addAction(_exact);
    secTool->addSeparator();
    secTool->addAction(actionConsulter);
    secTool->addSeparator();
    secTool->addAction(yeux);
    secTool->addAction(lunettes);
    secTool->addAction(balaiAct);
    secTool->addSeparator();
    secTool->addAction(quitAct);
    QVBoxLayout *vLayout = new QVBoxLayout(_trois);
    vLayout->setSpacing(0);
    vLayout->setMargin(0);
    _lemEdit = new QTextBrowser(this);
    _lemEdit->setOpenLinks(false);
    _lemEdit->setOpenExternalLinks(false);
    vLayout->addWidget(secTool);
    vLayout->addWidget(_lemEdit);

    _trois->setWindowTitle("Fenêtre de lemmatisation");
    _trois->setWindowIcon(QIcon(":/res/Eulexis.png"));
}

void MainWindow::montrer()
{
    if (_second->isVisible())
    {
        _second->hide();
        yeux->setChecked(false);
    }
    else
    {
        _second->show();
        yeux->setChecked(true);
    }
}

void MainWindow::montrer3()
{
    if (_trois->isVisible())
    {
        _trois->hide();
        lunettes->setChecked(false);
    }
    else
    {
        _trois->show();
        lunettes->setChecked(true);
        _lineEdit2->setFocus();
    }
}

void MainWindow::lemmatAlpha()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    _changements = true;
    if (!_trois->isVisible())
    {
        _trois->show();
        lunettes->setChecked(true);
    }
    QString txt = _texte->toPlainText();
    if (txt.isEmpty()) return;
    _lemEdit->clear();
    QStringList lm = txt.split(QRegExp("\\b"));
//    QStringList mots;
    QMultiMap<QString,QString> mots;
    // QMultiMap car deux mots différents peuvent avoir la même forme sans décoration.
    for (int i = 1; i < lm.size(); i+=2)
    {
        QString mot = lm[i];
        if (lm[i+1].startsWith("'") || lm[i+1].startsWith("´") || lm[i+1].startsWith("’"))
            mot.append("'");
        QString motNu = __lemmatiseur->beta2unicode(__lemmatiseur->nettoie(mot));
        if (!mot.contains(QRegExp("\\d")) && !mots.values().contains(mot))
            mots.insert(motNu,mot);
        // Je vérifie que le mot ne contient pas de chiffre et
        // qu'il ne figure pas encore dans les valeurs de mots.
//            mots.append(mot);
    }
//    mots.sort();
//    mots.removeDuplicates();
    int i = 0;
    QProgressDialog progr("Lemmatisation en cours...", "Arrêter", 0, mots.values().size()-1, _second);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    _trois->setUpdatesEnabled(false);
    foreach (QString mot, mots.values())
    {
        lemmatiser(mot);
        progr.setValue(i);
        i++; // Uniquement pour la barre de progression.
    }
    _trois->setUpdatesEnabled(true);
    _trois->repaint();
    QApplication::restoreOverrideCursor();
}

void MainWindow::lemmatTxt()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    _changements = true;
    if (!_trois->isVisible())
    {
        _trois->show();
        lunettes->setChecked(true);
    }
    QString txt = _texte->toPlainText();
    if (txt.isEmpty()) return;
    _lemEdit->clear();
    QStringList lm = txt.split(QRegExp("\\b"));
    QProgressDialog progr("Lemmatisation en cours...", "Arrêter", 0, lm.size()-1, _second);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    _trois->setUpdatesEnabled(false);
    for (int i = 1; i < lm.size(); i+=2)
    {
        QString mot = lm[i];
        progr.setValue(i);
        if (lm[i+1].startsWith("'") || lm[i+1].startsWith("´") || lm[i+1].startsWith("’"))
            mot.append("'");
        if (!mot.contains(QRegExp("\\d")))
            lemmatiser(mot);
    }
    _trois->setUpdatesEnabled(true);
    _trois->repaint();
//    _blabla->setText("");
    QApplication::restoreOverrideCursor();
}

void MainWindow::lAlld()
{
    __lemmatiseur->setCible(2);
}

void MainWindow::lAngl()
{
    __lemmatiseur->setCible(0);
}

void MainWindow::lFr()
{
    __lemmatiseur->setCible(1);
}

void MainWindow::avCons()
{
    raise();
    activateWindow();
    _lineEdit->setFocus();
}

void MainWindow::avTxt()
{
    if (_second->isVisible())
    {
        _second->raise();
        _second->activateWindow();
    }
    else
    {
        _second->show();
        yeux->setChecked(true);
    }
}

void MainWindow::avLem()
{
    if (_trois->isVisible())
    {
        _trois->raise();
        _trois->activateWindow();
        _lineEdit2->setFocus();
    }
    else
    {
        _trois->show();
        lunettes->setChecked(true);
    }
}

/**
 * @brief MainWindow::alerte
 * @param sauv : un booléen qui détermine le bouton par défaut
 * @return un booléen pour dire si on continue ou pas.
 *
 * Si on souhaite quitter le programme ou charger un nouveau texte
 * sans avoir sauvé le travail précédent, on affiche une boîte
 * de dialogue pour proposer de sauver le travail.
 *
 * Si on clique sur le bouton "Annuler", la fonction retourne false
 * et l'appelant doit en tenir compte (et ne rien faire).
 *
 * Si on clique sur "Sauver", la routine MainWindow::sauver() est appelée
 * avant de retourner la valeur true, sans chercher à savoir
 * si la sauvegarde a bien été faite.
 *
 * Si on clique sur "Ne pas sauver", on retourne true sans
 * autre forme de procès.
 *
 * Si le paramètre sauv est true, le bouton par défaut est "sauver".
 * Sinon, c'est "Ne pas sauver" qui est le bouton par défaut.
 *
 */
bool MainWindow::alerte(bool sauv)
{
    // Il y a des modifications non-sauvées.
    QMessageBox attention(QMessageBox::Warning,tr("Alerte !"),tr("Votre travail n'a pas été sauvé !"));
//        attention.setText(tr("Votre travail n'a pas été sauvé !"));
    QPushButton *annulerButton =
          attention.addButton(tr("Annuler"), QMessageBox::ActionRole);
    QPushButton *ecraserButton =
          attention.addButton(tr("Ne pas sauver"), QMessageBox::ActionRole);
    QPushButton *sauverButton =
          attention.addButton(tr("Sauver"), QMessageBox::ActionRole);
    if (sauv) attention.setDefaultButton(sauverButton);
    else attention.setDefaultButton(ecraserButton);
    attention.exec();
    if (attention.clickedButton() == annulerButton) return false;
    else if (attention.clickedButton() == sauverButton) sauver();
    return true;
}

void MainWindow::choixPolice()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, _txtEdit->font(), this);
    if (ok)
    {
        // font is set to the font the user selected
        _texte->setFont(font);
        _lemEdit->setFont(font);
        _txtEdit->setFont(font);
    }
}
/**
 * \fn
 * \brief
 *
 */
void MainWindow::exportPdf()
{
#ifndef QT_NO_PRINTER
    QString nf =
        QFileDialog::getSaveFileName(this, "Export PDF", QString(), "*.pdf");
    if (!nf.isEmpty())
    {
        if (QFileInfo(nf).suffix().isEmpty()) nf.append(".pdf");
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(nf);
        QTextEdit *tmpTE = new QTextEdit();
        tmpTE->setHtml(_lemEdit->toHtml());
//        tmpTE->append(textEditLem->toHtml());
        tmpTE->document()->print(&printer);
        delete tmpTE;
    }
#endif
}

/**
 * \fn void MainWindow::imprimer()
 * \brief Lance le dialogue d'impression pour la lemmatisation.
 */
void MainWindow::imprimer()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (_lemEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Imprimer le texte et le lexique"));
    if (dlg->exec() == QDialog::Accepted)
    {
        QTextEdit *tmpTE = new QTextEdit();
        tmpTE->setHtml(_lemEdit->toHtml());
//        tmpTE->append(textEditLem->toHtml());
        tmpTE->print(&printer);
        delete tmpTE;
    }
    delete dlg;
#endif
}

void MainWindow::chercher()
{
    bool ok;
    rech = QInputDialog::getText(this, tr("Recherche"), tr("Chercher :"),
                                 QLineEdit::Normal, rech, &ok);
    if (ok && !rech.isEmpty())
        rechercher();
/*    {
        if (!_texte->find(rech))
        {
            rech = QInputDialog::getText(this, tr("Chercher"),
                                         tr("Retour au début ?"),
                                         QLineEdit::Normal, rech, &ok);
            if (ok && !rech.isEmpty())
            {
                // Retourner au debut
                _texte->moveCursor(QTextCursor::Start);
                // Chercher à nouveau
                _texte->find(rech);
            }
        }
    }*/
}

void MainWindow::rechercher()
{
    if (rech.isEmpty()) return;
    bool ok;
    if (_second->isActiveWindow())
    {
        if (!_texte->find(rech))
        {
            QTextCursor tc = _texte->textCursor();
            rech = QInputDialog::getText(this, tr("Chercher"),
                                         tr("Retour au début ?"),
                                         QLineEdit::Normal, rech, &ok);
            if (ok && !rech.isEmpty())
            {
                // Retourner au debut
                _texte->moveCursor(QTextCursor::Start);
                // Chercher à nouveau
                if (!_texte->find(rech)) _texte->setTextCursor(tc);
            }
        }
    }
    else if (isActiveWindow())
    {
        if (!_txtEdit->find(rech))
        {
            QTextCursor tc = _txtEdit->textCursor();
            rech = QInputDialog::getText(this, tr("Chercher"),
                                         tr("Retour au début ?"),
                                         QLineEdit::Normal, rech, &ok);
            if (ok && !rech.isEmpty())
            {
                // Retourner au debut
                _txtEdit->moveCursor(QTextCursor::Start);
                // Chercher à nouveau
                if (!_txtEdit->find(rech)) _txtEdit->setTextCursor(tc);
            }
        }
    }
    else if (_trois->isActiveWindow())
    {
        if (!_lemEdit->find(rech))
        {
            QTextCursor tc = _lemEdit->textCursor();
            rech = QInputDialog::getText(this, tr("Chercher"),
                                         tr("Retour au début ?"),
                                         QLineEdit::Normal, rech, &ok);
            if (ok && !rech.isEmpty())
            {
                // Retourner au debut
                _lemEdit->moveCursor(QTextCursor::Start);
                // Chercher à nouveau
                if (!_lemEdit->find(rech)) _lemEdit->setTextCursor(tc);
            }
        }
    }
}
