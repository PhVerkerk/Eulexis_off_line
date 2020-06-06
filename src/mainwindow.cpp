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
            while ((mot.size() > 0) && mot[0].isPunct())
                mot = mot.mid(1);
            if (mot.isEmpty())
            {
                // Le "mot" ne contenait que de la ponctuation.
                // Je remonte au mot précédent...
                tc.movePosition(QTextCursor::WordLeft,QTextCursor::MoveAnchor,2);
                tc.select(QTextCursor::WordUnderCursor);
                mot = tc.selectedText();
//                qDebug() << mot;
//                return true;
            }
/* Je déplace ce morceau dans bulle()
            if (mot.endsWith("·") || mot.endsWith("·"))
                mot.chop(1);
            if (mot.endsWith("´"))
                mot.replace("´","'");
            if (mot.endsWith("’"))
                mot.replace("’","'");
                */
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
/*        if (st.endsWith("·") || st.endsWith("·")
                || st.endsWith("\u0375") || st.endsWith("\u037e"))
            st.chop(1);
            */
        st.replace(mainwindow->_reApostr,"'");
        QStringList lm = st.split(mainwindow->_reWordBoundary);
        if (lm.size() > 2)
        {
            st = lm[1];
            if (lm[2].startsWith("'")) st.append("'");
            mainwindow->lemmatiser(st);
        }
    }
    QTextEdit::mouseReleaseEvent(e);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    repertoire = QDir::homePath();
    createW();
    createSecond();
    createTrois();
    connecter();
    _msg << "%1 : word not found !\n<br/>";
    _msg << "%1 : mot non trouvé !\n<br/>";
    _msg << "%1 : Wort nicht gefunden !\n<br/>";
    _changements = false;
    _apostrophes = "'´΄’᾽";
    if (QFile::exists(_rscrDir + "apostrophes.txt"))
    {
        // Si le fichier existe, sa première ligne est l'ensemble des signes pouvant servir d'apostrophe.
        QFile fin(_rscrDir + "apostrophes.txt");
        fin.open(QIODevice::ReadOnly|QIODevice::Text);
        QByteArray ba = fin.readLine();
        _apostrophes = QString::fromUtf8(ba); // Ça devrait marcher aussi sur PC.
        _apostrophes.chop(1); // Le \n reste à la fin.
        fin.close();
    }
    _reApostr = QRegExp("["+_apostrophes+"]");
    _reWordBoundary = QRegExp("\\b");
//    qDebug() << _apostrophes.size() << _apostrophes;
    if (QFile::exists(_rscrDir + "entete_html.txt"))
    {
        // Si le fichier existe, sa première ligne est l'ensemble des signes pouvant servir d'apostrophe.
        QFile fin(_rscrDir + "entete_html.txt");
        fin.open(QIODevice::ReadOnly|QIODevice::Text);
        QByteArray ba = fin.readAll();
        _entete = QString::fromUtf8(ba); // Ça devrait marcher aussi sur PC.
        fin.close();
    }
    readSettings();
//    _second->show();
    dSetUp();
    _lineEdit->setFocus();
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
        settings.setValue("AbrBailly",AbrBailly->isChecked());
        settings.setValue("Bailly",Bailly->isChecked());
        settings.setValue("Allemand",langAlld->isChecked());
        settings.setValue("Anglais",langAngl->isChecked());
        settings.setValue("Français",langFr->isChecked());
        settings.endGroup();
        settings.beginGroup("options");
        // settings.setValue("police", font.family());
        settings.setValue("repertoire",repertoire);
        settings.setValue("beta",_beta->isChecked());
        settings.setValue("exact",_exact->isChecked());
        settings.setValue("TextiColor",actionTextiColor->isChecked());
        settings.setValue("Apostrophes",actionApostr->isChecked());
        settings.setValue("BOM",actionBOM->isChecked());
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
        AbrBailly->setChecked(settings.value("AbrBailly",true).toBool());
        Bailly->setChecked(settings.value("Bailly",true).toBool());
        langAlld->setChecked(settings.value("Allemand",false).toBool());
        if (langAlld->isChecked()) lAlld();
        langAngl->setChecked(settings.value("Anglais",false).toBool());
        if (langAngl->isChecked()) lAngl();
        langFr->setChecked(settings.value("Français",true).toBool());
        if (langFr->isChecked()) lFr();
    }
    settings.endGroup();
    settings.beginGroup("options");
    repertoire = settings.value("repertoire",repertoire).toString();
    _beta->setChecked(settings.value("beta",true).toBool());
    _exact->setChecked(settings.value("exact",true).toBool());
    actionTextiColor->setChecked(settings.value("TextiColor",false).toBool());
    actionApostr->setChecked(settings.value("Apostrophes",false).toBool());
    actionBOM->setChecked(settings.value("BOM",false).toBool());
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
    actionTxt2csv = new QAction("Txt2csv",this);
    actionTxt2csv->setObjectName(QStringLiteral("Convertir TXT en CSV"));
    actionTextiColor = new QAction("  Option TextiColor", this);
    actionTextiColor->setObjectName(QStringLiteral("Option pour sauver le texte colorisé"));
    actionTextiColor->setCheckable(true);
    actionTextiColor->setChecked(false);
    actionApostr = new QAction("  Option Apostrophes", this);
    actionApostr->setObjectName(QStringLiteral("Option pour mettre en évidence les apostrophes"));
    actionApostr->setCheckable(true);
    actionApostr->setChecked(false);
    actionBOM = new QAction("  Option BOM4ms", this);
    actionBOM->setObjectName(QStringLiteral("Option pour mettre BOM en début de fichier"));
    actionBOM->setCheckable(true);
    actionBOM->setChecked(false);
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
    auxAct = new QAction(QIcon(":res/help-browser.svg"), tr("aide"), this);
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
    AbrBailly = new QAction("Le Bailly abr.",this);
    AbrBailly->setCheckable(true);
    AbrBailly->setChecked(true);
    Bailly = new QAction("Le Bailly 2020",this);
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
    actAbrBailly = new QAction("Mettre à jour le Bailly abr.",this);
    actBailly = new QAction("Mettre à jour le Bailly 2020",this);
    actAnalyses = new QAction("Mettre à jour les analyses",this);
    actTrad = new QAction("Mettre à jour les traductions",this);
    actComInd = new QAction("Mettre à jour l'index commun",this);
    actVerif = new QAction("Vérifier les traductions",this);

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
    QString bulle = "Saisie en caractères latins\n";
    bulle.append("Substitutions : '*', '?', '2*', '2?' etc...\n");
    bulle.append("Expressions rationnelles...\n");
    bulle.append("Voir l'aide pour les détails");
    _lineEdit->setToolTip(bulle);
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

    // Restauration de la barre d'outils
    toolsRestoreAct = new QAction(tr("Restaurer les outils"),this);
    connect(toolsRestoreAct, SIGNAL(triggered()), this, SLOT(toolsRestore()));

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
    mainToolBar->addAction(auxAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(quitAct);
    mainToolBar->setFloatable(false);
    mainToolBar->setMovable(false);

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
    menuFichier->addAction(actionTxt2csv);
    menuFichier->addAction(actionTextiColor);
    menuFichier->addAction(actionApostr);
    menuFichier->addAction(actionBOM);
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
    menuDicos->addAction(AbrBailly);
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
    menuFenetre->addSeparator();
    menuFenetre->addAction(toolsRestoreAct);

    menuExtra->addAction(actAnalyses);
    menuExtra->addAction(actTrad);
    menuExtra->addSeparator();
    menuExtra->addAction(actLSJ);
    menuExtra->addAction(actPape);
    menuExtra->addAction(actAbrBailly);
    menuExtra->addAction(actBailly);
    menuExtra->addSeparator();
    menuExtra->addAction(actComInd);
//    menuExtra->addAction(actVerif);
    // Je commente cette ligne pour l'instant (le 30 mai 2020).

    menuAide->addAction(auxAct);
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
    connect(actionTxt2csv, SIGNAL(triggered()), this, SLOT(txt2csv()));
    connect(exportAct, SIGNAL(triggered()), this, SLOT(exportPdf()));
    connect(printAct, SIGNAL(triggered()), this, SLOT(imprimer()));
    connect(actionA_propos, SIGNAL(triggered()), this, SLOT(aPropos()));
    connect(yeux, SIGNAL(triggered()), this, SLOT(montrer()));
    connect(lunettes, SIGNAL(triggered()), this, SLOT(montrer3()));
    connect(balaiAct, SIGNAL(triggered()), this, SLOT(effaceRes()));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(auxAct, SIGNAL(triggered()), this, SLOT(auxilium()));

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
    connect(actVerif, SIGNAL(triggered()), this, SLOT(verifT()));
    connect(actAbrBailly, SIGNAL(triggered()), this, SLOT(majAB()));
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
        tr("<b>Eulexis</b> v.1<br/>\n"
           "<i>Lemmatiseur de Grec ancien<br/>\n inspiré de Collatinus</i><br/>\n"
           "Licence GPL, © Philippe Verkerk, 2017 <br/><br/>\n"
           "Merci à :<ul>\n"
           "<li>Yves Ouvrard</li>\n"
           "<li>Régis Robineau</li>\n"
           "<li>Jean-Paul Woitrain</li>\n"
           "<li>Philipp Roelli</li>\n"
           "<li>André Charbonnet (alias Chaerephon)</li>\n"
           "<li>Mark de Wilde</li>\n"
           "<li>Gérard Gréco, pour le magnifique Bailly</li>\n"
           "<li>Helma Dik et Logeion</li>\n"
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
/*    // Provisoirement j'utilise ce slot pour réparer les traductions
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",QDir::homePath(),"TXT files (*.txt)");
    if (!nomFichier.isEmpty())
    {
        if (__lemmatiseur->toInit()) __lemmatiseur->initData();
        __lemmatiseur->repairTransl(nomFichier);
    }
    */
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

void MainWindow::majAB()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier de l'abrégé du Bailly",QDir::homePath(),"Text files (*.txt)");
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

        if (QFile::exists(_rscrDir + "AbrBailly.csv"))
        {
            QFile::remove(_rscrDir + "AbrBailly.bak");
            QFile::rename(_rscrDir + "AbrBailly.csv",_rscrDir + "AbrBailly.bak");
        }
        // Si l'index existait, je le renomme en .bak
        if (QFile::exists(_rscrDir + nomCourt))
            QFile::remove(_rscrDir + nomCourt);
        QFile::copy(nomFichier,_rscrDir + nomCourt);
        // Je copie le fichier dans les ressources.

        __lemmatiseur->majAbrBailly(nomCourt);
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
    _second->raise();
}

void MainWindow::ouvrir()
{
    nouveau();
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",repertoire,"Text files (*.txt)");
    if (!nomFichier.isEmpty())
    {
        repertoire = QFileInfo (nomFichier).absolutePath ();
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
        QFileDialog::getSaveFileName(this, "Sauvegarder le travail en cours", repertoire, "*.txt");
    if (!nomFichier.isEmpty())
    {
        _changements = false;
        repertoire = QFileInfo (nomFichier).absolutePath ();
        if (QFileInfo(nomFichier).suffix().isEmpty()) nomFichier.append(".txt");
        QFile f(nomFichier);
        if (f.open(QFile::WriteOnly))
        {
//            QString txt = _texte->toPlainText();
            QString txt = _texte->toHtml();
            // Si je mets des couleurs avec TextiColor...
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
            if (plusPetit(LSJdata[0],f)) av = LSJdata[0];
            LSJdata.removeFirst();
            if (plusPetit(f,LSJdata[0])) ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" LSJ " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#lsj'>LSJ 1940 : </a></h3>\n");
            LSJdata[0].prepend("<h3><a name='#lsj'>LSJ 1940 : </a></h3>\n");
            if ((LSJdata.size() == 1) || longs(LSJdata))
                donnees << LSJdata;
            else donnees << LSJdata.join("\n<br />");
            // Si les articles sont courts, je les joins avant de passer à la suite.
        }
    }
    if (Pape->isChecked())
    {
        QStringList LSJdata = __lemmatiseur->consPape(f);
        if (!LSJdata.isEmpty())
        {
            if (plusPetit(LSJdata[0],f))
                if (plusPetit(av, LSJdata[0]) || (av == "")) av = LSJdata[0];
            LSJdata.removeFirst();
            if (plusPetit(f,LSJdata[0]))
                if (plusPetit(LSJdata[0], ap) || (ap == "")) ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" Pape " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#Pape'>Pape 1880 : </a></h3>\n");
            LSJdata[0].prepend("<h3><a name='#Pape'>Pape 1880 : </a></h3>\n");
            if ((LSJdata.size() == 1) || longs(LSJdata))
                donnees << LSJdata;
            else donnees << LSJdata.join("\n<br />");
            // Si les articles sont courts, je les joins avant de passer à la suite.
        }
    }
    if (AbrBailly->isChecked())
    {
        QStringList LSJdata = __lemmatiseur->consAbrBailly(f);
        if (!LSJdata.isEmpty())
        {
            if (plusPetit(LSJdata[0],f))
                if (plusPetit(av, LSJdata[0]) || (av == "")) av = LSJdata[0];
            LSJdata.removeFirst();
            if (plusPetit(f,LSJdata[0]))
                if (plusPetit(LSJdata[0],ap) || (ap == "")) ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" AbrBailly " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#Bailly'>Bailly abr. 1919 : </a></h3>\n");
            LSJdata[0].prepend("<h3><a name='#AbrBailly'>Bailly abr. 1919 : </a></h3>\n");
            if ((LSJdata.size() == 1) || longs(LSJdata))
                donnees << LSJdata;
            else donnees << LSJdata.join("\n<br />");
            // Si les articles sont courts, je les joins avant de passer à la suite.
        }
    }
    if (Bailly->isChecked())
    {
        QStringList LSJdata = __lemmatiseur->consBailly(f);
        if (!LSJdata.isEmpty())
        {
            if (plusPetit(LSJdata[0],f))
                if (plusPetit(av, LSJdata[0]) || (av == "")) av = LSJdata[0];
            LSJdata.removeFirst();
            if (plusPetit(f,LSJdata[0]))
                if (plusPetit(LSJdata[0],ap) || (ap == "")) ap = LSJdata[0];
            LSJdata.removeFirst();
            liens.append(" Bailly " + LSJdata[0]);
            LSJdata.removeFirst();
//            donnees.append("<h3><a name='#Bailly'>Bailly abr. 1919 : </a></h3>\n");
            LSJdata[0].prepend("<a name='#Bailly'><h3>Bailly 2020 : </h3></a>© Gérard Gréco\n");
            if ((LSJdata.size() == 1) || longs(LSJdata))
                donnees << LSJdata;
            else donnees << LSJdata.join("\n<br />");
            // Si les articles sont courts, je les joins avant de passer à la suite.
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
        QString res = liens;
        liens.prepend("<br />\n");
        _avant->setText(av);
        _apres->setText(ap);
        if (longs(donnees))
        {
            // res.append(donnees.join(liens));
            // Maintenant que j'ai les caractères de substitution,
            // je peux avoir plusieurs articles courts et un long.
            // C'est moche d'avoir la ligne de liens entre chaque article court.
            QString morceau = "";
            for (int i = 0; i < donnees.size() - 1; i++)
            {
                morceau.append(donnees[i]);
                if (morceau.size() > 5000 + 150 * morceau.count("href"))
                {
                    res.append(morceau);
                    res.append(liens);
                    morceau = "";
                }
                else morceau.append("<br />\n");
            }
            morceau.append(donnees.last());
            res.append(morceau);
        }
        else res.append(donnees.join("\n<br />"));
        /*    QString bla = "<html><head><style>";
            bla.append(".ital{font-style: italic;}");
            bla.append("</style></head><body>");
            bla.append("<p>miaou </p>");
            bla.append("<p class=\"ital\"> meuh</p>");
            bla.append("</body></html>");
            _texte->setHtml(bla);*/
        _txtEdit->setHtml(_entete + res + liens + "</body></html>");
//        _txtEdit->setHtml(res + liens);
    }
    _lineEdit->selectAll();
    _lineEdit->setFocus();
}

QString MainWindow::bulle(QString mot)
{
    // mot a été sélectionné par QTextCursor::WordUnderCursor
    // et je ne sais pas trop bien ce qui définit le mot.
    // Je me demande si je ne devrais pas appliquer
    // la bonne vieille méthode avec _reWordBoundary...
/*    if (mot.endsWith("·") || mot.endsWith("·")
            || mot.endsWith("\u0375") || mot.endsWith("\u037e"))
        mot.chop(1);
    // Quelques séparateurs grecs...
//    if (mot.endsWith("´"))
*/
    if (__lemmatiseur->toInit()) __lemmatiseur->initData();
    mot.replace(_reApostr,"'");
    QStringList lm = mot.split(_reWordBoundary);
    if (lm.size() < 3) return _msg[_lang].arg(mot);
    mot = lm[1];
    if (lm[2].startsWith("'")) mot.append("'");
    QStringList llem = __lemmatiseur->lemmatise(mot,_beta->isChecked());
    // Si j'ai passé une forme en grec avec décorations,
    // la première peut être en rouge si elle est exacte.
    QString res = "";
    if (llem.isEmpty()) res = _msg[_lang].arg(mot);
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
    if (__lemmatiseur->toInit()) __lemmatiseur->initData();
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
    if (llem.isEmpty()) res.append("</h4> " + _msg[_lang].arg(f));
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
    int i = _lineEdit->cursorPosition();
    QString lin = _lineEdit->text().trimmed();
    _lineEdit2->setText(lin);
    if (lin.contains(reLettres))
        lin = latin2greek(lin);
    if (!lin.isEmpty())
    {
        _lemmatiser->setText(lin);
        _lemmatiser2->setText(lin);
        actionConsulter->setToolTip("Consulter l'article " + lin);
    }
    _lineEdit->setCursorPosition(i);
}

void MainWindow::actualiser2()
{
    int i = _lineEdit2->cursorPosition();
    QString lin = _lineEdit2->text().trimmed();
    _lineEdit->setText(lin);
    if (lin.contains(reLettres))
        lin = latin2greek(lin);
    if (!lin.isEmpty())
    {
        _lemmatiser->setText(lin);
        _lemmatiser2->setText(lin);
        actionConsulter->setToolTip("Consulter l'article " + lin);
    }
    _lineEdit2->setCursorPosition(i);
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
    if (AbrBailly->isChecked()) opt += 4;
    if (Bailly->isChecked()) opt += 8;
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
//        qDebug() << lien << __lemmatiseur->beta2unicode(lien,false);
        QChar ch = lien[lien.size()-1];
        if (ch.category() == QChar::Number_DecimalDigit) lien.chop(1);
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
//    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if (__lemmatiseur->toInit()) __lemmatiseur->initData();
    _changements = true;
    QString txt = _texte->toPlainText();
    if (txt.isEmpty()) return;
    _lemEdit->clear();
    QString res;
    QString texteHTML = "";
    QStringList lm = txt.split(_reWordBoundary);
//    QStringList mots;
    QMultiMap<QString,QString> mots;
    // QMultiMap car deux mots différents peuvent avoir la même forme sans décoration.
    for (int i = 1; i < lm.size(); i+=2)
    {
        QString mot = lm[i];
//        if (lm[i+1].startsWith(_reApostr))
        if (lm[i+1].indexOf(_reApostr) == 0)
            mot.append("'");
        QString motNu = __lemmatiseur->beta2unicode(__lemmatiseur->nettoie(mot),false);
        if (!mot.contains(QRegExp("\\d")) && !mots.values().contains(mot))
            mots.insert(motNu,mot);
        // Je vérifie que le mot ne contient pas de chiffre et
        // qu'il ne figure pas encore dans les valeurs de mots.
//            mots.append(mot);
    }
//    mots.sort();
//    mots.removeDuplicates();
    int ratio = 1;
    int i = (mots.values().size()-1) / 100;
    while (ratio < i) ratio *= 2;
    int j = 1;
    QProgressDialog progr("Lemmatisation en cours...", "Arrêter", 0, mots.values().size() / ratio +5, _second);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    _trois->setUpdatesEnabled(false);
    i = 0;
    foreach (QString mot, mots.values())
    {
//        lemmatiser(mot);
        QStringList llem = __lemmatiseur->lemmatise(mot,_beta->isChecked());
        // Si j'ai passé une forme en grec avec décorations,
        // la première peut être en rouge si elle est exacte.
        res = "<h4>" + mot;
        if (llem.isEmpty()) res.append("</h4> " + _msg[_lang].arg(mot));
        else
        {
            res.append("</h4><ul>\n<li>");
            if (_exact->isChecked() && llem[0].startsWith("<span"))
                res.append(llem[0]);
            else res.append(llem.join("</li>\n<li>"));
            res.append("</li></ul>\n<br/>");
        }
        texteHTML.append(res);
        i++; // Uniquement pour la barre de progression.
        if (j * ratio < i)
        {
            j = i / ratio;
            progr.setValue(j);
            if (progr.wasCanceled())
                break;
            //... Stop !
        }
    }
    _lemEdit->setHtml(texteHTML);
    if (!_trois->isVisible())
    {
        _trois->show();
        lunettes->setChecked(true);
    }
    _trois->setUpdatesEnabled(true);
    _trois->repaint();
//    QApplication::restoreOverrideCursor();
}

void MainWindow::lemmatTxt()
{
//    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if (__lemmatiseur->toInit()) __lemmatiseur->initData();
    _changements = true;
    QString txt = _texte->toPlainText();
    if (txt.isEmpty()) return;
    _lemEdit->clear();
    QString res;
    QString texteHTML = "";
    bool bet = _beta->isChecked();
    bool ex = _exact->isChecked();
    QStringList lm = txt.split(_reWordBoundary);
    int ratio = 1;
    int i = (lm.size()-1) / 200;
    while (ratio < i) ratio *= 2;
    int j = 1;
    QProgressDialog progr("Lemmatisation en cours...", "Arrêter", 0, (lm.size() / ratio)+5, _second);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    _trois->setUpdatesEnabled(false);
    for (i = 1; i < lm.size(); i+=2)
    {
        QString mot = lm[i];
        if (j * ratio < i)
        {
            j = i / ratio;
            progr.setValue(j);
            if (progr.wasCanceled())
                break;
            //... Stop !
        }
//        if (lm[i+1].startsWith("'") || lm[i+1].startsWith("´") || lm[i+1].startsWith("΄")
  //              || lm[i+1].startsWith("’") || lm[i+1].startsWith("᾽"))
//        if (lm[i+1].startsWith(_reApostr))
        if (lm[i+1].indexOf(_reApostr) == 0)
            mot.append("'");
        if (!mot.contains(QRegExp("\\d")))
        {
            // lemmatiser(mot);
            // J'utilisais lemmatiser(mot), mais c'est absurde
            // car il insère les lemmatisations une à une à la fin de _lemEdit.
            // Il est bien plus malin de construire l'ensemble
            // des lemmatisations en HTML et de le balancer à la fin.
            QStringList llem = __lemmatiseur->lemmatise(mot,bet);
            // Si j'ai passé une forme en grec avec décorations,
            // la première peut être en rouge si elle est exacte.
            res = "<h4>" + mot;
            if (llem.isEmpty())
            {
                res.append("</h4> " + _msg[_lang].arg(mot));
                // Le mot est inconnu : en rouge et en gras !
                lm[i+1].prepend("</font></b>");
                lm[i-1].append("<b><font color=\"red\">");
            }
            else
            {
                res.append("</h4><ul>\n<li>");
                if (ex && llem[0].startsWith("<span"))
                {
                    res.append(llem[0]);
                    if (llem[0].contains("color:orange"))
                    {
                        lm[i+1].prepend("</b>");
                        lm[i-1].append("<b>");
                    }
                }
                else
                {
                    res.append(llem.join("</li>\n<li>"));
                    lm[i+1].prepend("</font></b>");
                    lm[i-1].append("<b><font color=\"blue\">");
                }
                res.append("</li></ul>\n<br/>");
            }
            texteHTML.append(res);
        }
    }
    _lemEdit->setHtml(texteHTML);
    if (!_trois->isVisible())
    {
        _trois->show();
        lunettes->setChecked(true);
    }
    _trois->setUpdatesEnabled(true);
    _trois->repaint();
    if (ex && actionTextiColor->isChecked())
    {
        // Je recompose le texte avec des couleurs.
        texteHTML = lm.join("");
        texteHTML.replace("\n","\n<br/>");
        _texte->setHtml(texteHTML);
    }
//    _blabla->setText("");
//    QApplication::restoreOverrideCursor();
}

void MainWindow::txt2csv()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",repertoire,"Text files (*.txt)");
    if (nomFichier.isEmpty()) return;
    if (__lemmatiseur->toInit()) __lemmatiseur->initData();
    bool beta = _beta->isChecked();
    bool ex = _exact->isChecked();
    // Plutôt que de tester ces boutons à chaque mot,
    // je définis des booléens locaux.
    repertoire = QFileInfo (nomFichier).absolutePath ();
    QString txt;
    QFile fEntree (nomFichier);
    if (fEntree.open (QFile::ReadOnly | QFile::Text))
    {
        QTextStream fluxL (&fEntree);
        fluxL.setCodec("UTF-8");
        txt = fluxL.readAll();
    }
    fEntree.close();
    if (txt.isEmpty()) return;
    nomFichier.replace(".txt",".csv");
    fEntree.setFileName(nomFichier);
    if (!fEntree.open (QFile::WriteOnly | QFile::Text)) return;
    QTextStream fluxL (&fEntree);
    fluxL.setCodec("UTF-8");
// Je suis prêt à écrire des choses dans fluxL
    if (actionBOM->isChecked()) fluxL << "\ufeff";
    fluxL << "Att\tSeq\tLine\tNumW\tForm\tLemma\tMeaning\tBetacode\tBetaWithout\n";

    QStringList lm = txt.split(_reWordBoundary);
    // lm est la liste des mots avec les séparateurs.
    int ratio = 1;
    int i = (lm.size()-1) / 200;
    while (ratio < i) ratio *= 2;
    int j = 1;
    QProgressDialog progr("Lemmatisation en cours...", "Arrêter", 0, (lm.size() / ratio) +5);
    // Je m'offre une marge de 5%.
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    QString res;
    int numMot = 0;
    int numLigne = 1;
    int nEl = 1;
    for (i = 1; i < lm.size(); i+=2)
    {
        // Les éléments impairs sont les mots.
        // Avant et après l'élément impair lm[i],
        // j'ai les deux séparateurs : je vais y mettre la mise en forme.
        QString mot = lm[i];
        if (j * ratio < i)
        {
            j = i / ratio;
            progr.setValue(j);
            if (progr.wasCanceled())
                break;
            //... Stop !
        }
        // ΄
//        if (lm[i+1].startsWith(_reApostr))
        if ((i > 1) && lm[i-1].contains("\n")) numLigne +=1;
        if (lm[i+1].indexOf(_reApostr) == 0)
        {
            mot.append("'");
            if (actionApostr->isChecked() && actionTextiColor->isChecked())
            {
                // David Carter voudrait que les apostrophes soient sur fond jaune.
                QString bla = lm[i+1].mid(0,1);
                bla.prepend("<span style=\"background-color:yellow;\">");
                bla.append("</span>");
                lm[i+1] = bla + lm[i+1].mid(1);
            }
        }
        if (mot.contains(QRegExp("\\d")))
        {
            fluxL << "\t" << nEl << "\t\t\t" << mot << "\n";
            // Si j'ai un nombre, je le mets dans la liste sans y toucher.
            nEl += 1;
        }
        else
        {
            numMot += 1;
            QStringList llem = __lemmatiseur->lem2csv(mot,beta);
            // Si j'ai passé une forme en grec avec décorations,
            // la première peut être en rouge si elle est exacte.
            // fluxL << mot << "\t";
            if (llem.isEmpty() && mot.endsWith("'"))
            {
                // C'est une élision non répertoriée : je peux essayer les 7 voyelles.
                // Un des problèmes est qu'il faudrait signaler ces tentatives.
                mot.chop(1);
                llem = __lemmatiseur->lem2csv(mot + "α",beta);
                llem.append(__lemmatiseur->lem2csv(mot + "ε",beta));
                llem.append(__lemmatiseur->lem2csv(mot + "η",beta));
                llem.append(__lemmatiseur->lem2csv(mot + "ι",beta));
                llem.append(__lemmatiseur->lem2csv(mot + "ο",beta));
                llem.append(__lemmatiseur->lem2csv(mot + "υ",beta));
                llem.append(__lemmatiseur->lem2csv(mot + "ω",beta));
                if (!llem.isEmpty()) llem[0].remove("<");
            }
            if (llem.isEmpty())
            {
                fluxL << "*\t" << nEl << "\t" << numLigne << "\t" << numMot << "\t" << mot << "\tUnknown\n";
                nEl += 1;
                // Le mot est inconnu : en rouge et en gras !
                lm[i+1].prepend("</font></b>");
                lm[i-1].append("<b><font color=\"red\">");
            }
            else
            {
                if (ex && llem[0].startsWith("<"))
                {
                    // Je n'ai que la première ligne à considérer.
                    res = llem[0];
                    bool exact = res.startsWith("<<");
                    if (!exact)
                    {
                        // Le mot est connu à la majuscule près : en gras !
                        lm[i+1].prepend("</b>");
                        lm[i-1].append("<b>");
                    }
                    res.remove("<");
                    QStringList eclats = res.split("\t");
                    // eclats[0] est la forme en grec ; les suivants sont les lemmes en BetaCode.
                    int taille = eclats.size();
                    for (int ii = 1; ii < taille; ii++)
                    {
                        if (taille > 2) fluxL << "*\t";
                        else fluxL << "\t";
                        fluxL << nEl << "\t" << numLigne << "\t" << numMot << "\t" << mot << "\t";
                        QString bla = __lemmatiseur->beta2unicode(eclats[ii], _beta->isChecked());
                        bla.replace("σ,","ς,");
                        fluxL << bla;
                        if (!exact) fluxL << " (" << eclats[0] << ")";
                        // Si la forme trouvée n'est pas exactement celle du texte,
                        // je donne la forme trouvée entre parenthèses.
                        if (eclats[ii].contains("-"))
                            fluxL << "\t" << __lemmatiseur->traduction(eclats[ii].section("-",1)) << "\t";
                        // Pour les composés explicites, on ne donne que la traduction de la racine.
                        else if (eclats[ii].contains(","))
                            fluxL << "\t" << __lemmatiseur->traduction(eclats[ii].section(",",1)) << "\t";
                        // S'il y a une virgule, mais pas de -, le lemme est après la virgule.
                        else
                            fluxL << "\t" << __lemmatiseur->traduction(eclats[ii]) << "\t";
                        fluxL << eclats[ii] << "\t" << __lemmatiseur->nettoie2(eclats[ii]) << "\n";
                        nEl += 1;
                    }
                }
                else //res = llem.join("\t");
                {
                    // Il y a des solutions aux diacritiques près :
                    // en bleu et en gras !
                    lm[i+1].prepend("</font></b>");
                    lm[i-1].append("<b><font color=\"blue\">");
//                    if (ex) lm[i-1].append("<b><font color=\"blue\">");
//                    else lm[i-1].append("<b><font color=\"green\">");
                    // Je mets le mot en vert si c'est une tentative de restitution d'élision.
                    // Je dois regrouper les lemmes de chaque forme approchée.
                    QMap<QString,QString> mapLem;
                    for (int iii = 0; iii < llem.size(); iii++)
                    {
                        res = llem[iii];
                        res.remove("<");
                        QStringList eclats = res.split("\t");
                        // eclats[0] est la forme en grec ; les suivants sont les lemmes en BetaCode.
                        for (int ii = 1; ii < eclats.size(); ii++)
                        {
                            if (mapLem.contains(eclats[ii]))
                                mapLem[eclats[ii]].append(", " + eclats[0]);
                            else mapLem[eclats[ii]] = eclats[0];
                        }
                    }
                    int taille = mapLem.keys().size();
                    foreach (QString lem, mapLem.keys())
                    {
                        if (taille > 1) fluxL << "*\t";
                        else fluxL << "\t";
                        fluxL << nEl << "\t" << numLigne << "\t" << numMot << "\t" << mot << "\t"
                              << __lemmatiseur->beta2unicode(lem, _beta->isChecked()) ;
                        fluxL << " (" << mapLem[lem] << ")";
                        fluxL << "\t" << __lemmatiseur->traduction(lem) << "\t";
                        fluxL << lem << "\t" << __lemmatiseur->nettoie2(lem) << "\n";
                        nEl += 1;
                    }
                }
            }
//            res.remove("<");
//            fluxL << res << "\n";
        }
    }
    fEntree.close();
    // J'en ai fini avec la création du CSV,
    // mais en cours de route, j'ai modifié la liste de mots.
    if (actionTextiColor->isChecked())
    {
        // L'option TextiColor est active : je sauve un fichier html.
        nomFichier.replace(".csv",".htm");
//        qDebug() << nomFichier;
        fEntree.setFileName(nomFichier);
        fEntree.open (QFile::WriteOnly | QFile::Text);
//        QTextStream fluxL (&fEntree);
//        fluxL.setCodec("UTF-8");
        // fluxL est déjà défini.
        fluxL << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
              << "<html>\n<head>\n"
              << "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"/>\n"
              << "<title>TextiColor by Eulexis</title>\n"
              << "<meta name=\"generator\" content=\"Eulexis (MacOSX)\"/>\n"
              << "<meta name=\"author\" content=\"Philippe \"/>\n"
              << "</head>\n<body>\n<p>";
        // Fin de l'entête HTML
        for (int i=0; i< lm.size() ; i++)
        {
            QString s = lm[i];
            s.replace("\n","</p>\n<p>");
            // Pour que les paragraphes restent des paragraphes.
            s.replace("</b> <b>"," ");
            // Si j'ai deux mots en gras de suite, je peux supprimer les balises.
            fluxL << s;
        }
        fluxL << "</p>\n</body></html>\n";
        fEntree.close();
    }
}

void MainWindow::lAlld()
{
    __lemmatiseur->setCible(2);
    _lang = 2;
}

void MainWindow::lAngl()
{
    __lemmatiseur->setCible(0);
    _lang = 0;
}

void MainWindow::lFr()
{
    __lemmatiseur->setCible(1);
    _lang = 1;
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
        QFont f = QFont(font.family(),font.pointSize());
        qDebug() << font.family() << _txtEdit->fontFamily();
        _texte->setFont(f);
        _lemEdit->setFont(f);
        QTextCursor tc(_lemEdit->document());
        QTextCharFormat tcf = tc.charFormat();
        tcf.setFont(f);
        tc.select(QTextCursor::Document);
        tc.mergeCharFormat(tcf);
        _txtEdit->setFont(f);
        qDebug() << _txtEdit->fontFamily();
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

bool MainWindow::plusPetit(QString g1, QString g2)
{
    return (latin2greek(__lemmatiseur->nettoie(g1)) < latin2greek(__lemmatiseur->nettoie(g2)));
}

bool MainWindow::longs(QStringList sl)
{
    for (int i=0; i<sl.size();i++)
        if (sl[i].size() > 4000 + 150 * sl[i].count("href"))
        {
//            qDebug() << sl[i].size() << sl[i].count("href") << sl[i];
            return true;
        }
    return false;
}

void MainWindow::auxilium()
{
    QDesktopServices::openUrl(QUrl("file:" + qApp->applicationDirPath() + "/Aide/index.html"));
}

void MainWindow::toolsRestore()
{
    mainToolBar->show();
}

void MainWindow::dSkip()
{
    // Rien ?
    dNext();
//    dVerif->close();
}

void MainWindow::dValid()
{
    // J'ai appuyé sur le bouton de validation
    // Sauver les données recueillies
    QString f = "%1\t%2\t%3\t%4\n";
    QString lg = f.arg(dLemme->text()).arg(dBeta->text()).arg(dOld->text()).arg(dLine->text());
    dFichier.write(lg.toUtf8());
    dNext();
//    dVerif->close();
}

void MainWindow::dSave()
{
    dValid();
    dFichier.close();
    dFichier.open(QIODevice::Append|QIODevice::Text);
}

void MainWindow::dSetUp()
{
//    qDebug() << "J'entre";
    dVerif = new QDialog(this);
    QLabel *tLemme = new QLabel("Lemme : ");
    QLabel *tBeta = new QLabel("Betacode : ");
    QLabel *tTrad = new QLabel("Traduction : ");
    dLemme = new QLabel();
    dBeta = new QLabel();
    dOld = new QLabel();
    dLine = new QLineEdit();
    dLine->setMinimumWidth(400);
//    qDebug() << dLine->minimumWidth();

    QPushButton *skipButton = new QPushButton(tr("Skip"));
    QPushButton *validButton = new QPushButton(tr("Valid"));
    QPushButton *saveButton = new QPushButton(tr("Save"));
    validButton->setDefault(true);
    connect(validButton, SIGNAL(clicked()), this, SLOT(dValid()));
    connect(skipButton, SIGNAL(clicked()), this, SLOT(dSkip()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(dSave()));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(skipButton);
    hLayout->addWidget(saveButton);
    hLayout->addWidget(validButton);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(tLemme,0,0,Qt::AlignRight);
    layout->addWidget(dLemme,0,1,Qt::AlignLeft);
    layout->addWidget(tBeta,1,0,Qt::AlignRight);
    layout->addWidget(dBeta,1,1,Qt::AlignLeft);
    layout->addWidget(tTrad,2,0,Qt::AlignLeft);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(layout);
    vLayout->addWidget(dOld);
    vLayout->addWidget(dLine);
    vLayout->addLayout(hLayout);

    dVerif->setLayout(vLayout);
//    qDebug() << dVerif->isModal();
//    qDebug() << "Je sors.";
}

void MainWindow::dNext()
{
    // J'affiche le prochain élément dans le dialogue de vérification
    bool notGot = true;
    QString ligne;
    QString clef;
    while (!dFlux.atEnd() && notGot)
    {
        ligne = dFlux.readLine ();
        if (!ligne.startsWith("!") && !ligne.isEmpty())
        {
            QStringList eclats = ligne.split("\t");
            if (eclats.size() > 3)
            {
                clef = __lemmatiseur->nettoie2(eclats[1]);
                if (clef[clef.size() - 1].isDigit()) clef.chop(1);
                clef.remove("-");
                consulter(clef);
                dLemme->setText(eclats[0]);
                dBeta->setText(eclats[1]);
                dOld->setText(eclats[2]);
                dLine->setText(eclats[3]);
                dLine->selectAll();
                notGot = false;
            }
            else qDebug() << ligne << eclats;
        }
    }

    if (dFlux.atEnd() && notGot)
    {
        dVerif->close();
        dFichier.close();
        dListe.close();
        // C'est fini !
    }
}
void MainWindow::verifT()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",QDir::homePath(),"CSV files (*.csv)");
    if (nomFichier.isEmpty()) return;

//    if (__lemmatiseur->toInit()) __lemmatiseur->initData();

    dListe.setFileName(nomFichier);
    dListe.open (QIODevice::ReadOnly|QIODevice::Text);
    dFlux.setDevice(&dListe);
    dFlux.setCodec("UTF-8");
    // Le fichier en entrée

    QString nom = nomFichier;
    nom.chop(4);
    nom.append("_revu.csv");
    dFichier.setFileName(nom);
    dFichier.open (QIODevice::WriteOnly|QIODevice::Text);
    // Le fichier en sortie.

    dNext();
    dVerif->show();
/*
    QString ligne;
    QString clef;
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (!ligne.startsWith("!") && !ligne.isEmpty())
        {
            QStringList eclats = ligne.split("\t");
            if (eclats.size() == 4)
            {
                clef = __lemmatiseur->nettoie2(eclats[1]);
                if (clef[clef.size() - 1].isDigit()) clef.chop(1);
                clef.remove("-");
                consulter(clef);
                dLemme->setText(eclats[0]);
                dBeta->setText(eclats[1]);
                dOld->setText(eclats[2]);
                dLine->setText(eclats[3]);
                dVerif->show();
            }
            else qDebug() << ligne << eclats;
        }
    }
    dVerif->close();
    dFichier.close();
    fListe.close();
    // C'est fini !*/
}

/* morceau récupéré dans LASLA_tagger v2.
void MainWindow::setDialFiche()
{
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text = new QLabel;
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text->setWordWrap(true);
    text->setText("Nouvelle fiche :");

    dFiche = new QDialog(this);
    QLabel *tForme = new QLabel("Forme : ");
    ficForme = new QLineEdit();
    QLabel *tLemme = new QLabel("Lemme : ");
    ficLemme = new QLineEdit();
    QLabel *tInd = new QLabel("Indice : ");
    ficIndice = new QLineEdit();
    QLabel *tCode = new QLabel("Code : ");
    ficCode9 = new QLineEdit();
    QPushButton *finButton = new QPushButton(tr("Appliquer"));
    connect(finButton, SIGNAL(clicked()), this, SLOT(paramFiche()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(icon,0,0,Qt::AlignCenter);
    layout->addWidget(text,0,1,Qt::AlignCenter);
    layout->addWidget(tForme,1,0,Qt::AlignRight);
    layout->addWidget(ficForme,1,1,Qt::AlignLeft);
    layout->addWidget(tLemme,2,0,Qt::AlignRight);
    layout->addWidget(ficLemme,2,1,Qt::AlignLeft);
    layout->addWidget(tInd,3,0,Qt::AlignRight);
    layout->addWidget(ficIndice,3,1,Qt::AlignLeft);
    layout->addWidget(tCode,4,0,Qt::AlignRight);
    layout->addWidget(ficCode9,4,1,Qt::AlignLeft);
    layout->addWidget(finButton,5,1,Qt::AlignRight);

    dFiche->setLayout(layout);

    ...
    QPushButton *annulerButton = new QPushButton(tr("Annuler"));
    QPushButton *retablirButton = new QPushButton(tr("Rétablir"));
    QPushButton *appliquerButton = new QPushButton(tr("Appliquer"));
    appliquerButton->setDefault(true);
    connect(appliquerButton, SIGNAL(clicked()), this, SLOT(setCouleurs()));
    connect(annulerButton, SIGNAL(clicked()), this, SLOT(paramFiche()));
    connect(retablirButton, SIGNAL(clicked()), this, SLOT(defCouleurs()));
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(annulerButton);
    hLayout->addWidget(retablirButton);
    hLayout->addWidget(appliquerButton);

    QGridLayout *layout4 = new QGridLayout;
    layout4->addWidget(icon4,0,0,Qt::AlignCenter);
    layout4->addWidget(text4,0,1,Qt::AlignCenter);
    layout4->addWidget(tc0,1,0,Qt::AlignRight);
    layout4->addWidget(c0,1,1,Qt::AlignLeft);
    layout4->addWidget(tc1,2,0,Qt::AlignRight);
    layout4->addWidget(c1,2,1,Qt::AlignLeft);
    layout4->addWidget(tc2,3,0,Qt::AlignRight);
    layout4->addWidget(c2,3,1,Qt::AlignLeft);
    layout4->addWidget(tc3,4,0,Qt::AlignRight);
    layout4->addWidget(c3,4,1,Qt::AlignLeft);
    layout4->addWidget(tc4,5,0,Qt::AlignRight);
    layout4->addWidget(c4,5,1,Qt::AlignLeft);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(layout4);
    vLayout->addLayout(hLayout);

    dCoul->setLayout(vLayout);

}
*/
