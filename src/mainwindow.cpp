#include "mainwindow.h"

/**
 * @file mainwindow.cpp
 * @brief définition des classes EditLatin et MainWindow
 */

/**
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

/**
 * @brief Créateur de la classe MainWindow
 * @param parent : le parent de cette classe
 *
 * Cette classe MainWindow crée l'interface graphique GUI pour Eulexis.
 */
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
    _histItem = 0;
    _historique.clear();
    _lineEdit->setFocus();
}

/**
 * @brief destructeur de la classe MainWindow
 *
 * @todo Actuellement vide.
 */
MainWindow::~MainWindow()
{

}

/**
 * @brief charge les analyses et les traductions
 *
 * Comme le chargement prend un peu de temps,
 * je mets un message dans la barre de "status"
 */
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
 *
 * S'il y a eu des changements depuis la dernière sauvegarde,
 * j'ouvre une boîte de dialogue qui demande une confirmation.
 * Voir MainWindow::alerte.
 * Lorsque la fermeture est confirmée, je sauve les paramètres
 * essentiels de l'application.
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

/**
 * @brief lecture des paramètres essentiels qui ont été
 * sauvegardés lors de la dernière fermeture.
 */
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
 * @brief Prépare la fenêtre principale.
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
    actionBackward =
        new QAction(QIcon(":/res/Gauche.svg"), tr("Revenir"), this);
    actionBackward->setDisabled(true);
    actionForward =
        new QAction(QIcon(":/res/Droite.svg"), tr("Avancer"), this);
    actionForward->setDisabled(true);

    fenCons = new QAction(QIcon(":/res/dicolitt.svg"), tr("Fenêtre de consultation"), this);
    fenLem = new QAction(QIcon(":/res/syntaxe.svg"), tr("Fenêtre de lemmatisation"), this);
    fenTxt = new QAction(QIcon(":/res/dicolem.svg"), tr("Fenêtre de texte"), this);
    balaiAct = new QAction(QIcon(":res/edit-clear.svg"),
                           tr("&Effacer les résultats"), this);
    effHistAct = new QAction(tr("Effacer l'historique"), this);
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
    mainToolBar->addAction(actionBackward);
    mainToolBar->addAction(actionForward);
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
    menuEdition->addAction(effHistAct);

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
    menuExtra->addAction(actVerif);
    // Je commente cette ligne pour l'instant (le 30 mai 2020).

    menuAide->addAction(auxAct);
    menuAide->addAction(actionA_propos);

    setWindowTitle(tr("Eulexis"));
    setWindowIcon(QIcon(":/res/Eulexis.png"));

    _rscrDir = qApp->applicationDirPath() + "/ressources/";
    __lemmatiseur = new Lemmat(_rscrDir);
}

/**
 * @brief connecte les @c actions aux @c slots
 */
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
    connect(actionBackward, SIGNAL(triggered()), this, SLOT(backward()));
    connect(actionForward, SIGNAL(triggered()), this, SLOT(forward()));
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
 * @brief Affiche une fenêtre de dialogue avec les remerciements.
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

/**
 * @brief mise à jour des analyses
 */
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

/**
 * @brief mise à jour des traductions
 */
void MainWindow::majT()
{
//    if (__lemmatiseur->toInit()) __lemmatiseur->initData();
//    __lemmatiseur->repairTransl("");
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

/**
 * @brief mise à jour du LSJ
 *
 * Cette routine ouvre une boîte de dialogue pour trouver
 * la **copie de travail** du dictionnaire et l'installer
 * à la place de la version précédente.
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine recopie la copie de travail
 * au bon endroit et appelle Lemmat::majLSJ qui
 * va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
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

/**
 * @brief mise à jour de l'abrégé du Bailly
 *
 * Cette routine ouvre une boîte de dialogue pour trouver
 * la **copie de travail** du dictionnaire et l'installer
 * à la place de la version précédente.
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine recopie la copie de travail
 * au bon endroit et appelle Lemmat::majAbrBailly qui
 * va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
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

/**
 * @brief mise à jour du Bailly
 *
 * Cette routine ouvre une boîte de dialogue pour trouver
 * la **copie de travail** du dictionnaire et l'installer
 * à la place de la version précédente.
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine recopie la copie de travail
 * au bon endroit et appelle Lemmat::majBailly qui
 * va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
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

/**
 * @brief mise à jour du Pape
 *
 * Cette routine ouvre une boîte de dialogue pour trouver
 * la **copie de travail** du dictionnaire et l'installer
 * à la place de la version précédente.
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine recopie la copie de travail
 * au bon endroit et appelle Lemmat::majPape qui
 * va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
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

/**
 * @brief mise à jour de l'index commun
 *
 * Renvoie directement sur Lemmat::indexCommun
 */
void MainWindow::majC()
{
    __lemmatiseur->indexCommun();
}

/**
 * @brief efface le texte dans la deuxième fenêtre
 */
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

/**
 * @brief ouvre un texte dans la deuxième fenêtre
 */
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

/**
 * @brief sauve le travail en cours
 * @param nomFichier : le nom sous lequel sauver le fichier
 *
 * Si le nom de fichier est vide, on ouvre une fenêtre de dialogue
 * pour choisir le nom du fichier.
 */
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

/**
 * @brief avance à l'article suivant des dictionnaires
 */
void MainWindow::avance()
{
    consulter(_avant->text());
}

/**
 * @brief recule à l'article précédent des dictionnaires
 */
void MainWindow::recule()
{
    consulter(_apres->text());
}

/**
 * @brief revient au dernier mot cherché dans les dictionnaires
 */
void MainWindow::backward()
{
    if (_histItem < _historique.size() - 1)
    {
        _histItem++;
        consulter(_historique[_histItem],false);
        actionForward->setDisabled(false);
        if (_histItem == _historique.size() - 1) actionBackward->setDisabled(true);
        else actionBackward->setDisabled(false);
    }
}

/**
 * @brief avance dans l'historique de consultation
 *
 * On ne peut avancer dans l'historique que si on est revenu en arrière
 * auparavant.
 */
void MainWindow::forward()
{
    if (_histItem > 0)
    {
        _histItem--;
        consulter(_historique[_histItem], false);
        actionBackward->setDisabled(false);
        if (_histItem == 0) actionForward->setDisabled(true);
        else actionForward->setDisabled(false);
    }
}

/**
 * @brief efface l'historique
 */
void MainWindow::effaceHist()
{
    _historique.clear();
    _histItem = 0;
    actionForward->setDisabled(true);
    actionBackward->setDisabled(true);
}

/**
 * @brief pour consulter les dictionnaires
 * @param f : le lemme recherché
 * @param ajoute : un booléen, @c true par défaut, pour ajouter le mot cherché dans l'historique
 *
 * Cette routine affiche, dans la fenêtre principale, les articles
 * des dictionnaires sélectionnés qui correspondent au lemme demandé.
 *
 * Quand on voyage dans l'historique, il ne faut pas y ajouter de nouveaux items
 * donc @a ajoute sera @c false.
 */
void MainWindow::consulter(QString f, bool ajoute)
{
    if (f.isEmpty()) f = _lineEdit->text().toLower();
    else _lineEdit->setText(f);
    if (ajoute)
    {
        if (_histItem > 0)
            for (int i = 0; i < _histItem; i++)
                _historique.removeFirst();
        // Si je suis dans l'historique et que je fais une nouvelle consultation,
        // je dois effacer ce que j'avais fait après la page présente.
        _historique.prepend(f);
        _histItem = 0;
        actionForward->setDisabled(true);
        if (_historique.size() > 1)
            actionBackward->setDisabled(false);
    }
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
        _txtEdit->setHtml(_entete + res + liens + "</body></html>");
//        _txtEdit->setHtml(res + liens);
    }
    _lineEdit->selectAll();
    _lineEdit->setFocus();
}

/**
 * @brief pour afficher une bulle d'aide avec la lemmatisation du mot sous le curseur
 * @param mot : le mot sous le curseur
 * @return le contenu de la bulle d'aide à afficher
 */
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

/**
 * @brief affiche la lemmatisation de la forme dans la troisième fenêtre
 * @param f : la forme à lemmatiser
 *
 * Si la forme @a f est vide, on prend le contenu de la ligne de saisie de la fenêtre.
 */
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

/**
 * @brief actualise le contenu de la deuxième ligne de saisie et celui des boutons associés.
 *
 * Il y a deux lignes de saisie dans Eulexis : une dans la fenêtre principale
 * et une deuxième dans la fenêtre de lemmatisation. Leur contenu est synchronisé.
 * Les boutons à droite de ces lignes de saisie donne ce contenu
 * en caractères grecs et il est également actualisé.
 *
 * @note La validation de l'une des lignes de saisie modifie
 * la fenêtre qui la contient (i.e. consultation des dictionnaires
 * pour la première et lemmatisation pour la seconde).
 * Un clic sur l'un des boutons modifie l'autre fenêtre
 * (i.e. consultation des dictionnaires pour un clic sur le second bouton
 * et lemmatisation pour un clic sur le premier).
 */
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

/**
 * @brief actualise le contenu de la première ligne de saisie et celui des boutons associés.
 *
 * Il y a deux lignes de saisie dans Eulexis : une dans la fenêtre principale
 * et une deuxième dans la fenêtre de lemmatisation. Leur contenu est synchronisé.
 * Les boutons à droite de ces lignes de saisie donne ce contenu
 * en caractères grecs et il est également actualisé.
 *
 * @note La validation de l'une des lignes de saisie modifie
 * la fenêtre qui la contient (i.e. consultation des dictionnaires
 * pour la première et lemmatisation pour la seconde).
 * Un clic sur l'un des boutons modifie l'autre fenêtre
 * (i.e. consultation des dictionnaires pour un clic sur le second bouton
 * et lemmatisation pour un clic sur le premier).
 */
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

/**
 * @brief convertit les caractères latins en caractères grecs
 * @param f : une chaine en caractères latins
 * @return la même chaine en caractères grecs
 *
 * Version moins élaborée que la conversion de betacode en unicode
 * puisqu'elle ne gère pas les signes diacritiques. Plus rapide ?
 * Voir Lemmat::beta2unicode
 */
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

/**
 * @brief groupe le choix des dictionnaires en un entier
 * @return un entier entre 0 et 15
 *
 * @deprecated ne semble pas utilisée
 */
int MainWindow::lireOptions()
{
    int opt=0;
    if (LSJ->isChecked()) opt++;
    if (Pape->isChecked()) opt += 2;
    if (AbrBailly->isChecked()) opt += 4;
    if (Bailly->isChecked()) opt += 8;
    return opt;
}

/**
 * @brief efface le contenu de la fenêtre de lemmatisation
 */
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

/**
 * @brief suit le lien
 * @param url : l'adresse du lien
 *
 * Dans la fenêtre de consultation des dictionnaires et
 * dans la fenêtre de lemmatisation, j'ai mis des liens
 * que je dois interpréter. Dans la fenêtre de lemmatisation,
 * les liens sont sur les lemmes et si on clique dessus,
 * ils conduisent à la consultation des dictionnaires pour
 * le lemme cliqué.
 *
 * Dans la fenêtre de consultation des dictionnaires,
 * c'est plus compliqué car il y a plusieurs types de liens :
 * * liens vers une ancre dans la page
 * * renvois vers un autre mot (donc consultation des dictionnaires)
 * * explicitation des références dans le LSJ
 *
 * @todo Dans le LSJ et dans le Bailly, j'ai l'explicitation
 * des références aux œuvres citées. Je ne l'ai pas pour
 * les deux autres dictionnaires.
 * Dans le LSJ, je cherche lors de l'affichage si je reconnais
 * une citation auquel cas je mets un lien qui l'explicite.
 * C'est un travail que je n'ai pas encore fait pour le Bailly.
 * Cela dit, je pourrais mettre des repères dans le fichier HTML
 * du Bailly, plutôt que de chercher a posteriori les références...
 */
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

    else if (lien.contains("//aller/"))
    {
        lien = lien.mid(13);
        lien.replace("%7C","|");
//        qDebug() << lien << __lemmatiseur->beta2unicode(lien,false);
        QChar ch = lien[lien.size()-1];
        if ((ch.category() == QChar::Number_DecimalDigit) && !lien.contains("#"))
            lien.chop(1);
        // Il y avait un problème si le renvoi contenait un indice d'homonymie.
        // Que j'ai corrigé, mais qui a empêché la consultation des articles du genre "stigma" = ϛ = #2
        consulter(__lemmatiseur->beta2unicode(lien,false));
    }
    // Normalement, c'est un # pour aller à une ancre.
    // Pour résoudre le conflit avec les "#1" etc... utilisés en betacode,
    // j'ai mis des "§" dans les liens pour faire les renvois.
}

/**
 * @brief valider ?
 *
 * @deprecated fonction vide et non utilisée
 */
void MainWindow::valider()
{
//    if (consAct->isChecked()) consulter();
//    else lemmatiser();
}

/**
 * @brief Création d'un EditLatin pour afficher le texte.
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

/**
 * @brief création d'une troisième fenêtre, pour la lemmatisation
 */
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

/**
 * @brief montrer/masquer la deuxième fenêtre (pour le texte)
 */
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

/**
 * @brief montrer/masquer la troisième fenêtre (pour la lemmatisation)
 */
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

/**
 * @brief lemmatisation du texte avec les mots dans l'ordre alphabétique
 *
 * Le texte contenu dans la deuxième fenêtre est décomposé en mots
 * qui sont rangés en ordre alphabétique sans tenir compte des signes diacritiques.
 * Le contenu de la troisième fenêtre (fenêtre de lemmatisation) est effacé
 * et remplacé par le résultat de la lemmatisation des mots du texte
 * préalablement ordonnés.
 *
 * @note L'option TextiColor n'a pas d'effet ici :
 * le texte de la deuxième fenêtre reste inchangé.
 * @todo reprendre dans Collatinus l'élimination des chiffres collés aux mots.
 */
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

/**
 * @brief lemmatisation du texte en respectant l'ordre des mots
 *
 * Le texte contenu dans la deuxième fenêtre est décomposé en mots.
 * Le contenu de la troisième fenêtre (fenêtre de lemmatisation) est effacé
 * et remplacé par le résultat de la lemmatisation des mots du texte.
 *
 * Si les deux options "formes exactes" et "TextiColor" sont activées,
 * le texte colorisé remplace le texte d'origine dans la deuxième fenêtre.
 * @note Moyennant un peu d'effort, on doit pouvoir lever la contrainte
 * sur l'option "formes exactes" pour que le texte soit colorisé.
 * @todo reprendre dans Collatinus l'élimination des chiffres collés aux mots.
 */
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

/**
 * @brief ouvre un fichier txt et produit un fichier csv avec sa lemmatisaton.
 *
 * Cette routine ouvre une fenêtre de dialogue pour choisir un fichier @c txt.
 * Ce texte est lu et lemmatisé avec seulement le lemme, pas l'analyse.
 * L'ensemble des lemmatisations est mis dans un fichier csv.
 *
 * @note Lorsque l'option "TextiColor" est activée, un fichier HTML
 * est également créé avec le texte colorisé.
 * @todo David Carter m'a signalé un problème lorsque le fichier d'analyses
 * donne la forme (avec longueurs) en plus du lemme.
 * Il ne voudrait conserver que le lemme.
 * Ici ou dans Lemmat::lem2csv
 */
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
    fluxL << "Att\tSeq\tLine\tNumW\tForm\tLemma\tMeaning\tFormBeta\tLemmaBeta\tLemmaWithout\n";

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
            fluxL << "\t" << nEl << "\t\t\t" << mot << "\t\t\t\t\t\n";
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
                fluxL << "*\t" << nEl << "\t" << numLigne << "\t" << numMot << "\t" << mot << "\tUnknown\t\t\t\t\n";
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
/*                        if (eclats[ii].contains("-"))
                            fluxL << "\t" << __lemmatiseur->traduction(eclats[ii].section("-",1)) << "\t";
                        // Pour les composés explicites, on ne donne que la traduction de la racine.
                        else if (eclats[ii].contains(","))
                            fluxL << "\t" << __lemmatiseur->traduction(eclats[ii].section(",",1)) << "\t";
                        // S'il y a une virgule, mais pas de -, le lemme est après la virgule.
                        else
                            // Je transfers ces tests dans la fonction de traduction
                            */
                            fluxL << "\t" << __lemmatiseur->traduction(eclats[ii]) << "\t";
                        // Ajouter la forme en betacode
                        fluxL << __lemmatiseur->uni2betacode(mot) << "\t";
                        if (eclats[ii].contains("-"))
                            fluxL << eclats[ii].section("-",1) << "\t"
                                  << __lemmatiseur->nettoie2(eclats[ii].section("-",1)) << "\n";
                        // Pour les composés explicites, on ne donne que la traduction de la racine.
                        else if (eclats[ii].contains(","))
                            fluxL << eclats[ii].section(",",1) << "\t"
                                  << __lemmatiseur->nettoie2(eclats[ii].section(",",1)) << "\n";
                        // S'il y a une virgule, mais pas de -, le lemme est après la virgule.
                        else
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
                        fluxL << nEl << "\t" << numLigne << "\t" << numMot << "\t" << mot << "\t";
                        QString bla = __lemmatiseur->beta2unicode(lem, _beta->isChecked());
                        bla.replace("σ,","ς,");
                        fluxL << bla;
                        fluxL << " (" << mapLem[lem] << ")";
                        fluxL << "\t" << __lemmatiseur->traduction(lem) << "\t";
                        // Ajouter la forme en betacode
                        fluxL << __lemmatiseur->uni2betacode(mot) << "\t";
                        if (lem.contains("-"))
                            fluxL << lem.section("-",1) << "\t"
                                  << __lemmatiseur->nettoie2(lem.section("-",1)) << "\n";
                        // Pour les composés explicites, on ne donne que la traduction de la racine.
                        else if (lem.contains(","))
                            fluxL << lem.section(",",1) << "\t"
                                  << __lemmatiseur->nettoie2(lem.section(",",1)) << "\n";
                        // S'il y a une virgule, mais pas de -, le lemme est après la virgule.
                        else
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
    QMessageBox::about(
        this, tr("File saved in the source folder !"),
        tr("<b>Tip:</b> if you are using Excel,\n"
           "open the file in Notepad <b>first</b>\n"
           "and save it as <b>UTF-8</b>.\n"
           "<b>Then</b> open it in Excel.\n"));
}

/**
 * @brief met l'allemand comme langue cible
 */
void MainWindow::lAlld()
{
    __lemmatiseur->setCible(2);
    _lang = 2;
}

/**
 * @brief met l'anglais comme langue cible
 */
void MainWindow::lAngl()
{
    __lemmatiseur->setCible(0);
    _lang = 0;
}

/**
 * @brief met le français comme langue cible
 */
void MainWindow::lFr()
{
    __lemmatiseur->setCible(1);
    _lang = 1;
}

/**
 * @brief met au premier plan la fenêtre de consultation
 */
void MainWindow::avCons()
{
    raise();
    activateWindow();
    _lineEdit->setFocus();
}

/**
 * @brief met au premier plan la fenêtre de texte (l'ouvre si nécessaire)
 */
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

/**
 * @brief met au premier plan la fenêtre de lemmatisation (l'ouvre si nécessaire)
 */
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
 * @brief Dialogue pour confirmer la fermeture
 * @param sauv : un booléen qui détermine le bouton par défaut
 * @return un booléen pour dire si on continue ou pas.
 *
 * Si on souhaite quitter le programme ou charger un nouveau texte
 * sans avoir sauvé le travail précédent, on affiche une boîte
 * de dialogue pour proposer de sauver le travail.
 * Elle propose trois boutons "Annuler", "Sauver" et "Ne pas sauver".
 *
 * * Si on clique sur le bouton "Annuler", la fonction retourne false
 * et l'appelant doit en tenir compte (et ne rien faire).
 *
 * * Si on clique sur "Sauver", la routine MainWindow::sauver() est appelée
 * avant de retourner la valeur true, sans chercher à savoir
 * si la sauvegarde a bien été faite.
 *
 * * Si on clique sur "Ne pas sauver", on retourne true sans
 * autre forme de procès.
 *
 * Si le paramètre @a sauv est true, le bouton par défaut est "Sauver".
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

/**
 * @brief ouvre une boîte de dialogue pour le choix de la police et l'applique
 */
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
 * \brief exporte en pdf le contenu de la fenêtre de lemmatisation
 *
 * Ouvre une fenêtre de dialogue pour choisir le nom du fichier à créer.
 * Le contenu de la fenêtre de lemmatisation est imprimé dans le fichier sus-nommé.
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

/**
 * @brief cherche une chaine dans la fenêtre active
 *
 * Cette routine ouvre une boîte de dialogue pour savoir
 * quelle chaine chercher et appelle MainWindow::rechercher.
 * En pratique, elle est appelée par l'item du menu Édition/Chercher
 * ou par Ctrl-F.
 */
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

/**
 * @brief poursuit la recherche
 *
 * La chaine à chercher est déjà connue.
 * Cette routine fait la recherche dans la fenêtre active.
 * En pratique, elle est appelée par l'item du menu
 * Édition/Chercher encore ou par Ctrl-G.
 * C'est aussi la deuxième étape de MainWindow::chercher.
 */
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

/**
 * @brief compare deux chaines en caractères grecs
 * sans tenir compte des signes diacritiques.
 * @param g1 : une chaine en caractères grecs
 * @param g2 : une chaine en caractères grecs
 * @return @c true si @a g1 vient avant @a g2
 *
 * La comparaison habituelle de deux chaines de caractères passe par
 * l'ordre des codes numériques associés aux lettres.
 * donc β (03B2) vient avant ά (1F71) or on voudrait ranger
 * tous les α, qu'ils aient un esprit ou un accent ou pas,
 * avant de passer à β.
 */
bool MainWindow::plusPetit(QString g1, QString g2)
{
    return (latin2greek(__lemmatiseur->nettoie(g1)) < latin2greek(__lemmatiseur->nettoie(g2)));
}

/**
 * @brief évalue la longueur de chaque article d'un même dictionnaire
 * @param sl : une liste de chaine contenant les articles
 * @return @c true si l'un des articles dépasse la longueur d'une page
 *
 * Cette fonction est utile pour savoir s'il faut intercaler la liste de liens
 * entre les différents articles de dictionnaire.
 * Si tous les articles d'un dictionnaire donné sont courts,
 * on peut les grouper sans mettre de ligne de liens entre deux articles.
 * Si dans l'ensemble des articles que l'on veut afficher il y en a des longs,
 * on va essayer de faire des groupes pour avoir une ligne de liens de temps en temps.
 */
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

/**
 * @brief ouvre l'index de l'aide utilisateur (dans le navigateur par défaut)
 */
void MainWindow::auxilium()
{
    QDesktopServices::openUrl(QUrl("file:" + qApp->applicationDirPath() + "/Aide/index.html"));
}

/**
 * @brief affiche à nouveau la barre d'outils
 */
void MainWindow::toolsRestore()
{
    mainToolBar->show();
}

/********
 * D'ici à la fin, j'ai mis les fonctions et routines qui servent
 * à la vérification des traductions.
 * Je ne sais pas encore si les laisser dans la version publiée...
 * ******/

/*void MainWindow::dSkip()
{
    // Rien ?
    opCode = -2;
    dNext();
}*/

/**
 * @brief annule les modifications faites pour le lemme en cours
 *
 * Ce @c slot est appelé quand on clique sur le bouton "Cancel".
 * Il ouvre une fenêtre de dialogue pour demander une confirmation.
 * Si on est revenu en arrière sur un lemme pour lequel le choix
 * a été validé, on peut revenir aux données initiales ou aux
 * données validées la première fois (et annuler seulement les dernières modifs).
 */
void MainWindow::dUndo()
{
    // J'ai appuyé sur le bouton d'annulation
    if (lg_Old.isEmpty()) // lgMod[numLem] = lgOrig[numLem];
    {
        QMessageBox attention(QMessageBox::Warning,tr("Annuler les modifs"),
                              tr("Vous allez perdre les modifs !"));
        QPushButton *revButton =
                attention.addButton(tr("Continuer"), QMessageBox::ActionRole);
        QPushButton *annulerButton =
                attention.addButton(tr("Annuler"), QMessageBox::ActionRole);
        attention.setDefaultButton(annulerButton);
        attention.exec();
        if (attention.clickedButton() == revButton)
        {
            lgMod[numLem] = lgOrig[numLem];
            opCode = 100;
            dAffiche(lgMod[numLem]);
        }
    }
    else
    {
        QMessageBox attention(QMessageBox::Warning,tr("Annuler les modifs"),
                              tr("Vous allez perdre les modifs !"));
        attention.setInformativeText("Quelles données reprendre ?");
        QPushButton *origButton =
                attention.addButton(tr("Originales"), QMessageBox::ActionRole);
        QPushButton *revButton =
                attention.addButton(tr("Revues"), QMessageBox::ActionRole);
        QPushButton *annulerButton =
                attention.addButton(tr("Annuler"), QMessageBox::ActionRole);
        attention.setDefaultButton(annulerButton);
        attention.exec();
        if (attention.clickedButton() == revButton)
        {
            lgMod[numLem] = lgOrig[numLem];
            opCode = 100;
            dAffiche(lgMod[numLem]);
        }
        else if (attention.clickedButton() == origButton)
        {
            lgMod[numLem] = lg_Old[numLem];
            opCode = 50;
            dAffiche(lgMod[numLem]);
        }
    }
}

/**
 * @brief sauve les choix déjà faits et propose de continuer ou d'arrêter.
 *
 * Ce @c slot est appelé quand on a cliqué sur le bouton "Save".
 * Il sauve l'ensemble des choix déjà faits, tout en conservant
 * l'état précédent du fichier de sortie (en le renommant en .bak).
 * Une fois que le fichier est sauvé, il ouvre une boîte de dialogue
 * pour demander ce que l'on souhaite faire. Le choix se résume à
 * continuer ou à arrêter.
 * @note A priori, il n'est pas utile de sauver le travail en cours.
 * La sauvegarde est automatique quand on arrive à la fin du fichier
 * et les fichiers sont suffisamment courts pour qu'ils puissent être
 * revus en une séance de travail. Toutefois, si on doit interrompre
 * son travail, il vaut toujours mieux sauver ce qui a déjà été fait.
 */
void MainWindow::dSave()
{
    QString lg = dNomFic;
    lg.replace(".csv",".bak");
    if (QFile::exists(dNomFic))
    {
        QFile::remove(lg);
        QFile::rename(dNomFic,lg);
    }
    // Si le fichier existait, je le renomme en .bak
    QFile dFichier(dNomFic);
    dFichier.open (QIODevice::WriteOnly|QIODevice::Text);
    for (int i = 0; i < nbLem; i++)
    {
        lg = lgMod[i] + "\n";
        dFichier.write(lg.toUtf8());
    }
    lg = dFichier.fileName() + "\t";
    lg.append(QDateTime::currentDateTime().toString("yy MM dd HH mm\n"));
    dFichier.write(lg.toUtf8());
    dFichier.close();
    if (numLem < nbLem)
    {
        QMessageBox attention(QMessageBox::Warning,tr("Fichier sauvé !"),
                              tr("Fichier sauvé !"));
        attention.setInformativeText("Que souhaitez-vous faire ?");
        QPushButton *fermerButton =
                attention.addButton(tr("Fermer"), QMessageBox::ActionRole);
        QPushButton *continuerButton =
                attention.addButton(tr("Continuer"), QMessageBox::ActionRole);
        attention.setDefaultButton(continuerButton);
        attention.exec();
        if (attention.clickedButton() == fermerButton) dVerif->hide();
        //dVerif->close();
    }
}

/**
 * @brief crée la fenêtre pour vérifier les traductions et connecte les boutons aux @c slots
 */
void MainWindow::dSetUp()
{
    dVerif = new QDialog(this);
    QLabel *tLemme = new QLabel("Lemme : ");
    QLabel *tBeta = new QLabel("Betacode : ");
    QLabel *tEul = new QLabel("Eulexis : ");
    QLabel *tEn = new QLabel("En : ");
    QLabel *tFr = new QLabel("Fr : ");
    QLabel *tDe = new QLabel("De : ");
    QLabel *tB = new QLabel("Bailly : ");
    QLabel *tC = new QLabel("Commentaire : ");
    dLemme = new QLabel();
    dBeta = new QLabel();
    dLemAp = new QLabel();
    dLemAv = new QLabel();
    dTrAp = new QLabel();
//    dTrAp->setMinimumWidth(240);
//    dTrAp->setMaximumWidth(241);
    dTrAv = new QLabel();
//    dTrAv->setMinimumWidth(240);
//    dTrAv->setMaximumWidth(241);
    dNb = new QLabel();
    dNum = new QLabel();
//    dOld = new QLabel();
//    dTrB = new QLineEdit();
    dTrB = new QTextEdit();
    dTrB->setMinimumWidth(500);
    dTrB->setMinimumHeight(50);
    dTrB->setMaximumHeight(100);

    dTrEn = new QLineEdit();
    dTrEn->setMinimumWidth(400);
    dTrEn->setMaximumWidth(401);
    dTrFr = new QLineEdit();
    dTrFr->setMinimumWidth(400);
    dTrDe = new QLineEdit();
    dTrDe->setMinimumWidth(400);
    dComment = new QLineEdit();
    dComment->setMinimumWidth(400);
//    qDebug() << dLine->minimumWidth();

    dLemB = new QComboBox();
    dLemB->setMinimumWidth(400);
    dLemB->setMinimumHeight(20);

    choixBailly = new QRadioButton("Valider trad Bailly",this);
    choixEulexis = new QRadioButton("Valider trad Eulexis",this);
    choixRemis = new QRadioButton("Remettre le choix",this);
    QHBoxLayout *hLayout0 = new QHBoxLayout;
    hLayout0->addWidget(choixBailly);
    hLayout0->addWidget(choixEulexis);
    hLayout0->addWidget(choixRemis);

    previousButton = new QPushButton(tr("Previous"));
    nextButton = new QPushButton(tr("Valid and Next"));
    QPushButton *undoButton = new QPushButton(tr("Undo"));
    QPushButton *saveButton = new QPushButton(tr("Save"));
    previousButton->setMinimumWidth(150);
    previousButton->setMaximumWidth(151);
    nextButton->setMinimumWidth(180);
    nextButton->setMaximumWidth(181);
    nextButton->setDefault(true);
    connect(nextButton, SIGNAL(clicked()), this, SLOT(dNext()));
    connect(previousButton, SIGNAL(clicked()), this, SLOT(dPrev()));
    connect(undoButton, SIGNAL(clicked()), this, SLOT(dUndo()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(dSave()));
    connect(dLemB, SIGNAL(currentIndexChanged(int)), this,
            SLOT(dChgLem(int)));

//    connect(dTrEn, SIGNAL(returnPressed()), this, SLOT(dNext()));
//    connect(dTrFr, SIGNAL(returnPressed()), this, SLOT(dNext()));
//    connect(dTrDe, SIGNAL(returnPressed()), this, SLOT(dNext()));
//    connect(dComment, SIGNAL(returnPressed()), this, SLOT(dNext()));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(previousButton);
    hLayout->addSpacing(10);
    hLayout->addWidget(saveButton);
    hLayout->addWidget(undoButton);
    hLayout->addSpacing(10);
    hLayout->addWidget(nextButton);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(dNum,0,0,Qt::AlignRight);
    layout->addWidget(dNb,0,1,Qt::AlignLeft);
    layout->addWidget(dLemAp,1,1,Qt::AlignRight);
    layout->addWidget(dLemAv,1,0,Qt::AlignLeft);
    layout->addWidget(dTrAp,2,1,Qt::AlignRight);
    layout->addWidget(dTrAv,2,0,Qt::AlignLeft);
    layout->addWidget(tLemme,3,0,Qt::AlignRight);
    layout->addWidget(dLemme,3,1,Qt::AlignLeft);
    layout->addWidget(tBeta,4,0,Qt::AlignRight);
    layout->addWidget(dBeta,4,1,Qt::AlignLeft);
    layout->addWidget(tEul,5,0,Qt::AlignLeft);

    QGridLayout *layout2 = new QGridLayout;
    layout2->addWidget(tEn,0,0,Qt::AlignRight);
    layout2->addWidget(dTrEn,0,1,Qt::AlignLeft);
    layout2->addWidget(tFr,1,0,Qt::AlignRight);
    layout2->addWidget(dTrFr,1,1,Qt::AlignLeft);
    layout2->addWidget(tDe,2,0,Qt::AlignRight);
    layout2->addWidget(dTrDe,2,1,Qt::AlignLeft);

    QGridLayout *layout3 = new QGridLayout;
    layout3->addWidget(tB,0,0,Qt::AlignLeft);
    layout3->addWidget(dLemB,0,1,Qt::AlignLeft);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(layout);
    vLayout->addLayout(layout2);
    vLayout->addLayout(layout3);
    vLayout->addWidget(dTrB);
    vLayout->addSpacing(20);
    vLayout->addWidget(tC);
    vLayout->addWidget(dComment);
    vLayout->addLayout(hLayout0);
    vLayout->addLayout(hLayout);

    dVerif->setLayout(vLayout);
//    qDebug() << dVerif->isModal();
//    qDebug() << "Je sors.";
}

/**
 * @brief valide le choix de traduction pour le lemme en cours
 * @return un booléen @c true si on veut passer au lemme suivant
 *
 * On a cliqué sur le bouton de validation, après avoir choisi
 * la nouvelle traduction du Bailly, la traduction actuelle d'Eulexis
 * ou en remettant le choix à plus tard. Dans ce dernier cas,
 * le programme demande une confirmation et retournera @c false
 * si on annule ce non-choix (auquel cas, on reste sur le lemme en cours).
 *
 * Cette routine regarde le choix fait (i.e. quel radio-bouton est validé),
 * et mets dans le fichier de sortie l'information correspondante.
 */
bool MainWindow::dValide()
{
    // Il faut sauver le contenu des lignes de saisie.
    QString nombre = "%1"; // Pour insérer facilement un nombre dans une chaine
    int changement = 0;
    if (choixRemis->isChecked())
    {
        // Vérifier que c'est bien ce que l'on veut faire
        QMessageBox attention(QMessageBox::Warning,tr("Choix remis !"),
                              tr("Choix remis !"));
        attention.setInformativeText("Êtes-vous sûr de vouloir remettre votre choix ?");
        QPushButton *fermerButton =
                attention.addButton(tr("Non"), QMessageBox::ActionRole);
        QPushButton *continuerButton =
                attention.addButton(tr("Oui"), QMessageBox::ActionRole);
        attention.setDefaultButton(continuerButton);
        attention.exec();
        if (attention.clickedButton() == fermerButton) return false;
        opCode -= 2;
    }
    else if (choixEulexis->isChecked())
    {
        // Vérifier que la traduction choisie n'est pas vide
        if (dTrFr->text().simplified().isEmpty())
        {
            QMessageBox attention(QMessageBox::Warning,tr("Traduction vide !"),
                                  tr("Traduction vide !"));
            attention.setInformativeText("Êtes-vous sûr de vouloir valider une traduction vide ?");
            QPushButton *fermerButton =
                    attention.addButton(tr("Non"), QMessageBox::ActionRole);
            QPushButton *continuerButton =
                    attention.addButton(tr("Oui"), QMessageBox::ActionRole);
            attention.setDefaultButton(fermerButton);
            attention.exec();
            if (attention.clickedButton() != continuerButton) return false;
        }
        opCode -= 1;
    }
    else
    {
        // Vérifier que la traduction choisie n'est pas vide
        if (dTrB->toPlainText().simplified().isEmpty())
        {
            QMessageBox attention(QMessageBox::Warning,tr("Traduction vide !"),
                                  tr("Traduction vide !"));
            attention.setInformativeText("Êtes-vous sûr de vouloir valider une traduction vide ?");
            QPushButton *fermerButton =
                    attention.addButton(tr("Non"), QMessageBox::ActionRole);
            QPushButton *continuerButton =
                    attention.addButton(tr("Oui"), QMessageBox::ActionRole);
            attention.setDefaultButton(fermerButton);
            attention.exec();
            if (attention.clickedButton() != continuerButton) return false;
        }
        opCode += indice;
    }
    QString ligne = elements[0] + "\t" + elements[1] + "\t" + elements[2] + "\t";
    QString clef = dTrEn->text();
    if (clef != elements[3]) changement += 1;
    if (clef.isEmpty()) ligne.append(elements[3] + "\t");
    else ligne.append(clef + "\t");
    // Il ne faudrait pas que la traduction anglaise se perde.
    // Les colonnes 4 et 5 sont plus compliquées
    trBailly[indice] = dTrB->toPlainText().simplified(); // copie des derniers changements
    if (trBailly.size() == 1)
    {
        // un seul élément
        if (trBailly[0] != elements[5]) changement += 8;
    }
    else
    {
        QStringList eclats = elements[5].split("@");
        // Les eclats correspondent aux traductions avant édition
        int aj = 8;
        for (int i = 0; i < eclats.size(); i++)
        {
            if (trBailly[i] != eclats[i]) changement += aj;
            aj = 2 * aj;
        }
    }
    clef = dTrFr->text();
    if (clef != elements[6]) changement += 2;
    if ((opCode > -1) && !clef.isEmpty() && !lemBailly.contains("fr_Eulexis"))
    {
        // La place de l'ancienne traduction va être utilisée pour la nouvelle
        lemBailly.append("fr_Eulexis");
        trBailly.append(clef);
    }
    ligne.append(lemBailly.join("@") + "\t");
    ligne.append(trBailly.join("@") + "\t");
    // La colonne 6 est la nouvelle trad Fr
/*    if (opCode == -2) ligne.append("\t"); // Skip
    else*/
    // Je garde la traduction fr d'Eulexis pour opCode == -1 et -2.
    if (opCode < 0) ligne.append(clef + "\t");
        // Ancienne traduction du Bailly déjà dans clef
    else ligne.append(dTrB->toPlainText().simplified() + "\t");
    clef = dTrDe->text();
    if (clef != elements[7]) changement += 4;
    ligne.append(clef + "\t");
    ligne.append(elements[8] + "\t");
    if ((elements.size() > 10))
    {
        // Le fichier CSV d'origine a 10 colonnes.
        // Si on reprend un fichier déjà revu, j'en ai ajouté 2
        // (en réalité, supprimé une et ajouté trois).
        // Je souhaite garder ces colonnes supplémentaires !
        if ((changement > 0) || (opCode != -1) || (dComment->text() != elements[11]))
        {
            ligne.append(nombre.arg(opCode) + "@" + elements[9] + "\t");
            ligne.append(nombre.arg(changement) + "@" + elements[10] + "\t");
            ligne.append(dComment->text() + "@" + elements[11] + "\t");
        }
        else
        {
            ligne.append(elements[9] + "\t");
            ligne.append(elements[10] + "\t");
            ligne.append(elements[11] + "\t");
        }
    }
    else
    {
        ligne.append(nombre.arg(opCode) + "\t");
        ligne.append(nombre.arg(changement) + "\t");
        ligne.append(dComment->text() + "\t");
    }
    if ((changement > 0) && (opCode != -1))
        ligne.append(QDateTime::currentDateTime().toString("yy MM dd HH mm"));
    else if (elements.size() > 10) ligne.append(elements[12]);
    // Si je ne fais pas de changement, je n'ai pas à indiquer d'heure.
    // Je termine la ligne avec l'heure.
    lgMod[numLem] = ligne;
//    dFichier.write(ligne.toUtf8());
    opCode = 0;
    return true;
}

/**
 * @brief affiche un nouveau lemme et ses traductions dans la fenêtre de vérification.
 * @param ligne : la ligne du fichier contenant les informations à vérifier.
 */
void MainWindow::dAffiche(QString ligne)
{
    elements = ligne.split("\t");
    if (elements.size() > 9)
    {
        QString clef = elements[2]; // C'est le lemme en betacode.
        if (clef.contains("-")) clef = __lemmatiseur->reconcil(clef);
        clef = __lemmatiseur->nettoie2(clef);
        if (clef[clef.size() - 1].isDigit()) clef.chop(1);
        consulter(clef);
        // J'ouvre les dicos à l'article demandé.
        dLemme->setText(elements[1]);
        dBeta->setText(elements[2]);
        dTrEn->setText(elements[3]);
        dTrFr->setText(elements[6]);
        dTrDe->setText(elements[7]);
        if (elements[4].isEmpty())
        {
            lemBailly.clear();
            trBailly.clear();
            lemBailly << "";
            trBailly << "";
            // Je ne sais pas ce que ferait un split sur une chaine vide...
        }
        else
        {
            lemBailly = elements[4].split("@");
            trBailly = elements[5].split("@");
        }
        dTrB->setEnabled(true);
        choixBailly->setEnabled(true);
        dTrB->setText(trBailly[0]);
        dLemB->clear();
        indice = 0;
        dLemB->addItems(lemBailly);
        dLemB->setCurrentIndex(0);
        if (elements.size() > 11)
        {
            dComment->setText(elements[11].section("@",0,0));
            if ((elements[9].mid(0,2) == "-2") ||
                    (elements[9].mid(0,2) == "98") ||
                    (elements[9].mid(0,2) == "48"))
                choixRemis->setChecked(true);
            else choixEulexis->setChecked(true);
            // Si j'ai 12 éléments et que le choix a déjà été fait,
            // je l'ai mis dans la traduction d'Eulexis.
            // Si j'ai remis mon choix (avec ou sans undo),
            // je reste sur un non-choix.
        }
        else
        {
            dComment->clear();
            choixBailly->setChecked(true);
            // Par défaut, le Bailly est choisi...
        }
        if (ligne.startsWith("*")) dLemB->setStyleSheet(css_vert);
        else if (ligne.startsWith("?"))
        {
            if (lemBailly.size() == 1) dLemB->setStyleSheet(css_orange);
            else dLemB->setStyleSheet(css_rouge);
        }
        else if (ligne.startsWith("!"))
        {
            dLemB->setStyleSheet(css_gris);
            choixEulexis->setChecked(true);
            // Je n'ai pas de traductions du Bailly.
            // Le choix par défaut est la traduction d'Eulexis.
            dTrB->setEnabled(false);
            choixBailly->setEnabled(false);
        }
        else dLemB->setStyleSheet(css_blanc);
        QString nb = "%1";
        dNum->setText(nb.arg(numLem + 1));
        if (numLem == 0)
        {
            previousButton->setText("");
            dLemAv->setText("");
            dTrAv->setText("");
            previousButton->setEnabled(false);
        }
        else
        {
            previousButton->setText(lgMod[numLem - 1].section("\t",1,1) + " <");
            dLemAv->setText(lgMod[numLem - 1].section("\t",1,1) + " <");
            dTrAv->setText(lgMod[numLem - 1].section("\t",3,3));
            previousButton->setEnabled(true);
        }
        if (numLem == nbLem - 1)
        {
            nextButton->setText("Valid and Save");
            dLemAp->setText("");
            dTrAp->setText("");
        }
        else
        {
            nextButton->setText("Valide > " + lgMod[numLem + 1].section("\t",1,1));
            dLemAp->setText("> " + lgMod[numLem + 1].section("\t",1,1));
            dTrAp->setText(lgMod[numLem + 1].section("\t",3,3));
        }
    }
}

/**
 * @brief valide le choix de traduction et passe au lemme suivant
 *
 * C'est ce qui se passe quand on clique sur le bouton par défaut :
 * on valide le choix et on passe au lemme suivant.
 * Quand on a atteint la fin du fichier, l'ensemble des informations
 * est sauvé et la fenêtre de vérification est fermée.
 */
void MainWindow::dNext()
{
    if (!dValide()) return;
    // J'ai fini de sauver le résultat du lemme précédent.
    // J'affiche le prochain élément dans le dialogue de vérification
    numLem++;
    if (numLem < nbLem) dAffiche(lgMod[numLem]);
    else
    {
        dSave();
        // C'est fini !
        dVerif->hide();
//        dVerif->close();
    }
}

/**
 * @brief revient au lemme considéré précédemment
 *
 * Ce @c slot est appelé quand on clique sur le bouton "Previous".
 * On revient alors au lemme qui précède et on peut corriger les traductions.
 */
void MainWindow::dPrev()
{
//    dValide();
    // Je ne valide pas quand je reviens en arrière.
    // J'affiche le prochain élément dans le dialogue de vérification
    if (numLem > 0)
    {
        numLem--;
        dAffiche(lgMod[numLem]);
    }
}

/**
 * @brief change la traduction du Bailly, quand on choisit un autre lemme dans ce dictionnaire
 * @param i : le numéro de l'item choisi dans la comboBox
 *
 * Un des buts de la vérification des traductions est de déméler les homonymes.
 * Initialement, les lemmes avaient une traduction anglaise tirée du LSJ.
 * Pas toujours heureuse dans le cas des homonymes.
 * Lorsque j'ai dépouillé le Bailly 2020, j'ai mis en regard avec les lemmes
 * d'Eulexis tous les homonymes trouvés dans le Bailly qui pourraient correspondre.
 * Ils sont dans une comboBox et l'utilisateur doit choisir quel lemme du Bailly
 * correspond à celui proposé (la traduction anglaise aide à choisir).
 */
void MainWindow::dChgLem(int i)
{
    // J'ai changé de lemme dans la comboBox.
    if ((i > -1) && (i < trBailly.size()))
    {
        trBailly[indice] = dTrB->toPlainText().simplified(); // Je sauve les changements
        indice = dLemB->currentIndex();
//        qDebug() << i << indice;
        dTrB->setText(trBailly[indice]);
    }
}

/**
 * @brief lance la vérification des traductions
 *
 * Ce @c slot est appelé par l'item de menu Extra/Vérifier les traductions.
 * Si la fenêtre de vérification est déjà ouverte, il se contente de la mettre
 * au premier plan.
 * Sinon, il ouvre une fenêtre de dialogue pour choisir le fichier de traductions à vérifier.
 * Si le fichier a déjà été (partiellement) revu (voir MainWindow::dSave),
 * les données d'origine sont chargées aussi (si le fichier est trouvé) et
 * le travail est repris là où il a été interrompu.
 *
 * @note On trouvera un mode d'emploi détaillé sur
 * https://github.com/DigiClass/alpheios-french-dictionary/blob/master/docs/mode_d_emploi.md
 */
void MainWindow::verifT()
{
    if (dVerif->isVisible())
    {
        dVerif->raise();
        return;
    }
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Lire le fichier",QDir::homePath(),"CSV files (*.csv)");
    if (nomFichier.isEmpty()) return;

    lgOrig.clear();
    lgMod.clear();
    lg_Old.clear();
    QFile dListe(nomFichier);
    dListe.open (QIODevice::ReadOnly|QIODevice::Text);
    dFlux.setDevice(&dListe);
    dFlux.setCodec("UTF-8");
    // Le fichier en entrée
    while (!dFlux.atEnd())
    {
        QString ligne = dFlux.readLine();
        if (!ligne.startsWith("\t") && !ligne.isEmpty())
        {
            lgOrig.append(ligne);
            lgMod.append(ligne);
        }
    }
    dListe.close();
    nbLem = lgMod.size();
    numLem = 0;
    dNomFic = nomFichier;
    if (nomFichier.contains("_revu"))
    {
        nbLem--;
        // La dernière ligne contient d'autres infos.
        while ((numLem < nbLem) && (lgMod[numLem].count("\t") > 10)) numLem++;
        // Si j'ai plus de 10 Tabs, le lemme a déjà été revu.
        if (numLem == nbLem) numLem = 0;
        // Si tout a été revu, je reviens au début.
        // Est-ce que j'ai envie de charger aussi le fichier non-revu ?
        nomFichier.remove("_revu");
        if (QFile::exists(nomFichier))
        {
            dListe.setFileName(nomFichier);
            dListe.open (QIODevice::ReadOnly|QIODevice::Text);
            while (!dFlux.atEnd())
            {
                QString ligne = dFlux.readLine();
                if (!ligne.startsWith("\t") && !ligne.isEmpty())
                    lg_Old.append(ligne);
            }
            dListe.close();
        }
    }
    else
    {
        dNomFic.chop(4);
        dNomFic.append("_revu.csv");
        // Je crée le nom du fichier de sortie.
    }

/*    QString nom = nomFichier;
    nom.chop(4);
    nom.append("_revu.csv");
    dFichier.setFileName(nom);
    dFichier.open (QIODevice::WriteOnly|QIODevice::Text);
    nom.append("\n");
    dFichier.write(nom.toUtf8());
    // Le fichier en sortie.
*/
    dAffiche(lgMod[numLem]);
    QString nb = "/ %1";
    dNb->setText(nb.arg(nbLem));
    dVerif->show();
    dVerif->move(x() + width() - dVerif->width()/2, y() + (height() - dVerif->height())/2);

}
