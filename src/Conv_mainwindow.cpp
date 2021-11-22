#include "Conv_mainwindow.h"

/**
 * \file Conv_mainwindow.cpp
 * \brief CMainWindow = GUI de Greek_converter
 * \author Philippe Verkerk
 * \version 1
 * \date 2018
 *
 * Greek_converter est un petit programme qui permet
 * de convertir du betacode en unicode et réciproquement.
 */

/**
 * @brief créateur de la classe CMainWindow
 * @param parent : le parent
 *
 * Cette classe définit la fenêtre d'interface de Greek_converter.
 * Greek_converter est un petit programme qui permet
 * de convertir du betacode en unicode et réciproquement.
 */
CMainWindow::CMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    _repertoire = QDir::homePath();
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

/**
 * @brief destructeur de la classe CMainWindow
 */
CMainWindow::~CMainWindow()
{

}

/**
 * \fn void CMainWindow::closeEvent(QCloseEvent *event)
 * \brief ferme l'application.
 */
void CMainWindow::closeEvent(QCloseEvent *event)
{
        event->accept();
}

/**
 * @brief crée la fenêtre principale
 *
 * Prépare la fenêtre principale qui est ouverte
 * ainsi qu'une fenêtre de dialogue pour le cas des fichiers CSV
 * qui ne sera affichée que plus tard, quand ce sera nécessaire.
 *
 */
void CMainWindow::createW()
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
    actionNorm = new QAction("Normalise",this);
    actionNorm->setObjectName(QStringLiteral("Normalise"));
    actionA_propos = new QAction("À propos",this);
    actionA_propos->setObjectName(QStringLiteral("actionA_propos"));
    quitAct = new QAction(QIcon(":/res/power.svg"), tr("&Quitter"), this);
    quitAct->setStatusTip(tr("Quitter l'application"));
    quitAct->setShortcut(
        QKeySequence(tr("Ctrl+Q")));  // QKeySequence::Quit inopérant

    _b2u = new QPushButton("Betacode to Unicode",this);
    _b2u->setMinimumWidth(150);
    _b2u->setMaximumWidth(250);
    _b2u->setToolTip("Convert a file from Betacode to UTF-8");
    _u2b = new QPushButton("Unicode to Betacode",this);
    _u2b->setMinimumWidth(150);
    _u2b->setMaximumWidth(250);
    _u2b->setToolTip("Convert a file from UTF-8 to Betacode");

    _norm = new QPushButton("Normalise Unicode",this);
    _norm->setMinimumWidth(150);
    _norm->setMaximumWidth(250);
    _norm->setToolTip("Normalise the unicode content of a file");

    _betaButton = new QToolButton(this);
    _betaButton->setText("ϐ");
    _betaButton->setCheckable(true);
    _betaButton->setChecked(false);
    _betaButton->setToolTip("Distinguer les beta");

    _capsButton = new QToolButton(this);
    _capsButton->setText("Caps");
    _capsButton->setCheckable(true);
    _capsButton->setChecked(false);
    _capsButton->setToolTip("Capital betacode");

    _autoName = new QToolButton(this);
    _autoName->setText("autoName");
    _autoName->setCheckable(true);
    _autoName->setChecked(true);
    _autoName->setToolTip("Nomme automatiquement le fichier de sortie");

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
    mainToolBar->addWidget(_norm);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(_autoName);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(_betaButton);
    mainToolBar->addWidget(_capsButton);
    mainToolBar->addSeparator();
    mainToolBar->addAction(quitAct);

    menuBar->addAction(menuFichier->menuAction());
    menuFichier->addAction(actionB2U);
    menuFichier->addAction(actionU2B);
    menuFichier->addSeparator();
    menuFichier->addAction(actionNorm);
    menuFichier->addSeparator();
    menuFichier->addAction(actionA_propos);
    menuFichier->addSeparator();
    menuFichier->addAction(quitAct);

    setWindowTitle(tr("Greek Converter"));
    setWindowIcon(QIcon(":/res/Eulexis.png"));

    // Préparer le dialogue pour les fichiers CSV
    dialCSV = new QDialog(this);
    group1 = new QButtonGroup(this);
    group2 = new QButtonGroup(this);
    rbAll = new QRadioButton("All",this);
    rbRange = new QRadioButton("Range :",this);
    rbAll->setChecked(true);
    rbRange->setChecked(false);
    group1->addButton(rbAll);
    group1->addButton(rbRange);
    rbTab = new QRadioButton("Tab",this);
    rbComma = new QRadioButton("Comma",this);
    rbTab->setChecked(true);
    rbComma->setChecked(false);
    group2->addButton(rbTab);
    group2->addButton(rbComma);
    range = new QLineEdit(this);
    range->setText("1,3-5");
    QLabel *tTitre = new QLabel("Convert a CSV or TSV file.");
    QLabel *tCS = new QLabel("Column separator : ");

    QPushButton *okButton = new QPushButton(tr("OK"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(fermeDial()));
    QPushButton *finButton = new QPushButton(tr("Annuler"));
    connect(finButton, SIGNAL(clicked()), this, SLOT(annuleDial()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(tTitre,0,0,Qt::AlignCenter);
    layout->addWidget(rbAll,1,0,Qt::AlignLeft);
    layout->addWidget(rbRange,2,0,Qt::AlignLeft);
    layout->addWidget(range,2,1,Qt::AlignLeft);
    layout->addWidget(tCS,3,0,Qt::AlignRight);
    layout->addWidget(rbTab,4,0,Qt::AlignLeft);
    layout->addWidget(rbComma,4,1,Qt::AlignLeft);
    layout->addWidget(finButton,5,0,Qt::AlignRight);
    layout->addWidget(okButton,5,1,Qt::AlignRight);

    dialCSV->setLayout(layout);


}

/**
 * @brief Connecte les @c actions aux @c slots
 */
void CMainWindow::connecter()
{

    connect(actionB2U, SIGNAL(triggered()), this, SLOT(bet2uni()));
    connect(actionU2B, SIGNAL(triggered()), this, SLOT(uni2bet()));
    connect(actionNorm, SIGNAL(triggered()), this, SLOT(normalise()));
    connect(_b2u, SIGNAL(clicked()), this, SLOT(bet2uni()));
    connect(_u2b, SIGNAL(clicked()), this, SLOT(uni2bet()));
    connect(_norm, SIGNAL(clicked()), this, SLOT(normalise()));
    connect(actionA_propos, SIGNAL(triggered()), this, SLOT(aPropos()));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

}

/**
 * @brief Affiche une fenêtre de dialogue avec les remerciements.
 *
 */
void CMainWindow::aPropos()
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

/**
 * @brief Ouvre un fichier texte ou csv
 * @return un booléen true si le fichier est chargé
 *
 * Cette routine ouvre une boîte de dialogue pour lire
 * un fichier @c txt ou @c csv.
 * Si l'opération a été annulée ou que le fichier n'a pas pu être lu,
 * cette fonction retourne le booléen @c false.
 * Si tout c'est bien passé, le booléen @c true est renvoyé.
 */
bool CMainWindow::ouvrir()
{
    QString nomFichier =
            QFileDialog::getOpenFileName(this, "Open file",_repertoire,"Text files (*.txt;*.csv)");
    if (!nomFichier.isEmpty())
    {
        _texte.clear();
        QFile fEntree (nomFichier);
        if (fEntree.open (QFile::ReadOnly | QFile::Text))
        {
            _txtEdit->append("Loading file : "+nomFichier+"\n");
            _texte = QString::fromUtf8(fEntree.readAll());
            QFileInfo fi(nomFichier);
            _repertoire = fi.absolutePath ();
            _nom = fi.baseName();
            _isCSV = (fi.suffix() == "csv");
            return true;
        }
    }
    return false;
}

/**
 * @brief Sauve le texte converti
 * @param nomFichier : le nom du fichier à créer
 *
 * Si le bouton CMainWindow::_autoName est activé,
 * le nom du fichier à créer est généré à partir du nom du fichier ouvert.
 * Si le @a nomFichier est vide, cette routine ouvre une fenêtre de dialogue
 * pour choisir le nom du fichier à créer.
 */
void CMainWindow::sauver(QString nomFichier)
{
    if (nomFichier.isEmpty()) nomFichier =
        QFileDialog::getSaveFileName(this, "Save file as :", _repertoire, "*.txt;*.csv");
    if (!nomFichier.isEmpty())
    {
        QFileInfo fi(nomFichier);
        _repertoire = fi.absolutePath ();
        if (fi.suffix().isEmpty())
        {
            if (_isCSV) nomFichier.append(".csv");
            else nomFichier.append(".txt");
        }
        QFile f(nomFichier);
        if (f.open(QFile::WriteOnly))
        {
            _txtEdit->append("Saving file : "+nomFichier+"\n");
            f.write(_texte.toUtf8());
            f.close();
        }
    }
}

/**
 * @brief Convertit du betacode en unicode
 * @param f : une chaine de caractères en betacode
 * @param beta : un booléen pour distinguer les deux bêtas
 * @return la chaine convertie en unicode
 */
QString CMainWindow::beta2unicode(QString f, bool beta)
{
    // Transf le betacode en unicode
        // Le sigma final
    f.append(" ");
    f.prepend(" ");
    f = f.toLower();
    f.replace(reSigmaFinal,"ς\\1");
    if (!beta)
    {
        f.replace("*b","Β");
        f.replace("b","β");
    }
    else f.replace(reBetaInitial,"\\1β");

    for (int i=0; i<_beta.size();i++)
        f.replace(_beta[i],_uni[i]);

    return f.mid(1,f.size()-2);
}

/**
 * @brief Convertit de l'unicode en betacode
 * @param f : une chaine de caractères en unicode
 * @return la chaine convertie en betacode
 *
 * La norme betacode prévoit que les lettres employées sont en majuscule.
 * Certains (dont je suis) ont trouvé ça peu lisible et ont tout basculé en minuscule.
 * Le bouton CMainWindow::_capsButton permet de respecter la forme initiale en majuscule.
 * Par défaut, il est désactivé.
 */
QString CMainWindow::uni2betacode(QString f)
{
    // Transf l'unicode en betacode
    for (int i=0; i<_beta.size();i++)
        f.replace(_uni[i],_beta[i]);
    if (_capsButton->isChecked()) return f.toUpper();
    return f;
}

/**
 * @brief Ouvre un fichier, le convertit de betacode en unicode et le sauve
 *
 * Si le fichier ouvert est un CSV, une fenêtre de dialogue supplémentaire s'ouvre
 * pour choisir le séparateur (virgule ou tabulation) et les colonnes à convertir.
 * Dans cette fenêtre, on a deux boutons "Annuler" et "OK" :
 * * "Annuler" annule toute l'opération et le fichier n'est pas converti
 * * "OK" valide les choix faits (voir aussi CMainWindow::listEntiers).
 */
void CMainWindow::bet2uni()
{
    // Ouvrir, transformer et sauver.

    if (!ouvrir()) return;
    _annule = false;
    if (_isCSV) dialCSV->exec();
    if (_annule) return;
    if (!_isCSV || rbAll->isChecked())
    {
        _txtEdit->append("Converting to unicode.\n");
        _texte = beta2unicode(_texte,_betaButton->isChecked());
    }
    else
    {
        // Je dois décomposer chaque ligne pour ne convertir
        // que les colonnes demandées.
        _txtEdit->append("Converting column "+range->text()+" to unicode\n");
        QString sep = ",";
        if (rbTab->isChecked()) sep = "\t";
        QList<int> valeurs = listEntiers(range->text());
        QStringList lignes = _texte.split("\n");
        for (int i=0 ; i<lignes.size() ; i++)
        {
            QStringList colonnes = lignes[i].split(sep);
            for (int j=0 ; j<valeurs.size() ; j++)
                if (valeurs[j]<colonnes.size())
                    colonnes[valeurs[j]] =
                            beta2unicode(colonnes[valeurs[j]],_betaButton->isChecked());
            lignes[i] = colonnes.join(sep);
        }
        _texte = lignes.join("\n");
    }
    if (_autoName->isChecked())
        sauver(_repertoire + "/" + _nom + "_conv");
    else sauver();
    _txtEdit->append("Done !\n");
}

/**
 * @brief Ouvre un fichier, le convertit d'unicode en betacode et le sauve
 *
 * Si le fichier ouvert est un CSV, une fenêtre de dialogue supplémentaire s'ouvre
 * pour choisir le séparateur (virgule ou tabulation) et les colonnes à convertir.
 * Dans cette fenêtre, on a deux boutons "Annuler" et "OK" :
 * * "Annuler" annule toute l'opération et le fichier n'est pas converti
 * * "OK" valide les choix faits (voir aussi CMainWindow::listEntiers).
 */
void CMainWindow::uni2bet()
{
    // Ouvrir, transformer et sauver.
    if (!ouvrir()) return;
    _annule = false;
    if (_isCSV) dialCSV->exec();
    if (_annule) return;
    if (!_isCSV || rbAll->isChecked())
    {
        _txtEdit->append("Converting to betacode.\n");
        _texte = uni2betacode(_texte);
    }
    else
    {
        // Je dois décomposer chaque ligne pour ne convertir
        // que les colonnes demandées.
        _txtEdit->append("Converting column "+range->text()+" to betacode\n");
        QString sep = ",";
        if (rbTab->isChecked()) sep = "\t";
        QList<int> valeurs = listEntiers(range->text());
        QStringList lignes = _texte.split("\n");
        for (int i=0 ; i<lignes.size() ; i++)
        {
            QStringList colonnes = lignes[i].split(sep);
            for (int j=0 ; j<valeurs.size() ; j++)
                if (valeurs[j]<colonnes.size())
                    colonnes[valeurs[j]] =
                            uni2betacode(colonnes[valeurs[j]]);
            lignes[i] = colonnes.join(sep);
        }
        _texte = lignes.join("\n");
    }
    if (_autoName->isChecked())
        sauver(_repertoire + "/" + _nom + "_conv");
    else sauver();
    _txtEdit->append("Done !\n");
}

/**
 * @brief Ouvre un fichier, en normalise l'unicode et le sauve
 */
void CMainWindow::normalise()
{
    if (!ouvrir()) return;
    if (!_isCSV || (_texte.size() < 300000))
    {
        _txtEdit->append("Normalisation of Unicode.\n");
        _texte = _texte.normalized(QString::NormalizationForm_C);
    }
    else
    {
        // Pour les gros fichiers, je traite chaque ligne.
        _txtEdit->append("Normalisation of Unicode, line by line.\n");
        QStringList lignes = _texte.split("\n");
        for (int i=0 ; i<lignes.size() ; i++)
        {
            lignes[i] = lignes[i].normalized(QString::NormalizationForm_C);
        }
        _texte = lignes.join("\n");
    }
    if (_autoName->isChecked())
        sauver(_repertoire + "/" + _nom + "_conv");
    else sauver();
    _txtEdit->append("Done !\n");
}

/**
 * @brief ferme et valide la fenêtre de dialogue CSV
 */
void CMainWindow::fermeDial()
{
    _annule = false;
    dialCSV->close();
}

/**
 * @brief annule et ferme le dialogue CSV
 */
void CMainWindow::annuleDial()
{
    _annule = true;
    dialCSV->close();
}

/**
 * @brief Explicite la liste d'entiers
 * @param le : une chaine représentant une liste d'entiers
 * @return la liste d'entiers
 *
 * La boîte de dialogue pour la conversion de fichiers CSV
 * permet de choisir les colonnes à convertir.
 * Comme souvent, on peut donner des numéros séparés par des virgules
 * ou spécifier des plages de valeurs avec un tiret.
 *
 * Exemples :
 * * "2,4" va convertir le contenu de la 2e colonne et celui de la 4e.
 * * "2-4" va convertir le contenu des colonnes 2, 3 et 4.
 * * On peut combiner les deux notations comme "1,3-5"
 * qui va convertir le contenu des colonnes 1, 3, 4 et 5.
 *
 * @attention La première colonne s'appelle bien 1.
 * @attention Une plage de valeurs donne les limites (incluses)
 * dans l'ordre naturel : "min-max" avec **min < max**.
 * @attention L'ordre est en revanche indifférent pour les valeurs séparées
 * par des virgules : "4,2" fera exactement la même chose que "2,4".
 */
QList<int> CMainWindow::listEntiers(QString le)
{
    QList<int> li;
    QStringList eclats = le.split(",");
    for (int i=0 ; i<eclats.size() ; i++)
        if (eclats[i].contains("-"))
        {
            // C'est un intervalle
            for (int j=eclats[i].section("-",0,0).toInt() - 1 ;
                 j<eclats[i].section("-",1,1).toInt() ; j++)
                li << j;
        }
        else li << eclats[i].toInt() - 1;
    return li;
}
