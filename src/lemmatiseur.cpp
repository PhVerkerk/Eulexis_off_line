#include "lemmatiseur.h"

/**
 * @file lemmatiseur.cpp
 * @brief définition de la classe Lemmat qui sert à lemmatiser les formes grecques
 */

/**
 * @brief Créateur de la classe Lemmat
 * @param rep : le chemin complet du dossier contenant les ressources
 *
 * J'initialise ici quelques variables et je lis le tableau de conversion
 * entre le betacode et l'unicode.
 * Je ne lis plus les index des dictionnaires ni les listes de formes et de traductions.
 * Partant du constat que c'est surtout pour consulter les dictionnaires
 * que l'on utilise Eulexis, je ne lis les listes de formes et de traductions
 * que lorsque l'on demande une lemmatisation ou une traduction.
 * On attend moins au démarrage, mais on attend une deuxième fois
 * quand on demande une analyse.
 */
Lemmat::Lemmat(QString rep)
{
    _rscrDir = rep;
    _cible = 1; // Français
    QFile fListe (rep + "betunicode_gr.csv");
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
/*    lireAnalyses();
    lireTraductions();
qDebug() << _formes.size() << _trad.size() << _beta.size() << _uni.size();
    lireLSJ();
    lireAbrBailly();
    lirePape();*/
    reLettres = QRegExp("[A-Za-z]");
    rePonct = QRegExp("([\\.?!;:,\\)\\(])");
    _toInit = true; // Je ne charge plus les analyses au démarrage.
    _maxList = 20; // Nombre max de réponses données à une requête contenant des caractères de substitution.
}

/**
 * @brief lit les index des dictionnaires
 *
 * Pour essayer de faire paraître le temps de chargement moins long,
 * je ne charge les index des dictionnaires que lorsque j'ai créé
 * la fenêtre principale du programme. Je ne suis pas sûr que ça marche.
 */
void Lemmat::lireData()
{
    lireLSJ();
    lireAbrBailly();
    lireBailly();
    lirePape();
}

/**
 * @brief lemmatise une forme
 * @param f : la forme qui peut être en caractères latins ou grecs
 * @param beta : booléen pour distinguer les deux bêtas.
 * @return Une liste de chaines de caractères au format HTML
 *
 * Je cherche la forme sans accent ni diacritique dans ma liste
 * Lemmat::_formes qui contient les formes de Diogenes
 * avec leurs lemmatisation(s) et analyse(s).
 * Toutes les formes qui partagent les mêmes caractères sans tenir
 * compte des diacritiques sont groupées dans Lemmat::_formes
 * et je vais donc les séparer pour faire les items de la liste.
 * Une forme particulière peut, à son tour, venir de plusieurs lemmes
 * et/ou avoir plusieurs analyses possibles. Donc chaque item contient
 * une liste avec ces lemmatisations et analyses.
 *
 * Si la forme a été donnée en caractères grecs avec ses signes diacritiques
 * et que, parmi les formes trouvées, il y en a une qui correspond exactement,
 * elle sera placée en début de liste et la forme sera en rouge.
 * Si elle ne diffère que par une majuscule, elle sera en orangé.
 */
QStringList Lemmat::lemmatise(QString f,bool beta)
{
    QStringList llem;
    if (f.isEmpty()) return llem;
    QString f_gr = "";
    if (!f.contains(reLettres))
    {
        f_gr = uni2betacode(f); // Je garde la forme grecque en Betacode.
        f = nettoie2(f_gr);
/*        if (!beta) f_gr.replace("ϐ","β");
        else
        {
            f_gr.replace("β","ϐ");
            if (f_gr.startsWith("ϐ"))
                f_gr = "β" + f_gr.mid(1);
            // La forme grecque sera comparée au résultat de la conversion beta2unicode.
            // Je dois donc mettre ou enlever les beta intérieurs.
        } */
//        f_gr = uni2betacode(f_gr);
        // Je préfère convertir la forme grecque en betacode pour éviter le pb tonos / oxia.
        // Mark de Wilde m'a signalé que l'accent grave remplace souvent l'accent aigu.
        // De fait, il n'y a que 18 accents graves dans analyses_gr, tjs sur les lemmes.
        f_gr.replace("\\","/");
        // Il peut y avoir un accent d'enclitique : aigu sur la dernière syllabe.
        if ((f_gr.count('/') == 2) ||
                ((f_gr.count('/') == 1) && (f_gr.count('=') == 1) && (f_gr.indexOf('/') > f_gr.indexOf('='))))
            f_gr.remove(f_gr.lastIndexOf('/'),1);
    }
//    qDebug() << beta << f << f_gr;
    if (!_formes.contains(f))
    {
//        llem << "Forme non trouvée / Not found";
        return llem;
    }
//    QFile fGrAn (_rscrDir + "greek-analyses.txt");
//    fGrAn.open (QIODevice::ReadOnly|QIODevice::Text);
    QString analyses = _formes.value(f);
//    qDebug() << f << analyses;
//    foreach (qint64 p, _formes.values(f))
    foreach (QString ligne, analyses.split("}\t"))
    {
//        fGrAn.seek(p);
//        QString ligne = fGrAn.readLine();
        QStringList ecl = ligne.split("{");
        QString mot = ecl[0].mid(0,ecl[0].size()-1);
        ligne = beta2unicode(mot,beta);
//        qDebug() << mot << f_gr << (mot == f_gr);
        if (mot == f_gr)
        {
            ligne.prepend("<span style='color:red'>");
            ligne.append("</span>");
        }
        else if (f_gr.startsWith("*") && !mot.startsWith("*"))
        {
            // La forme du texte commence avec une majuscule (en début de phrase ?).
            QString m = f_gr.mid(1);
            if (m.indexOf(reLettres) > 0)
            {
                QString esprit = m.mid(0,m.indexOf(reLettres));
                m = m.mid(m.indexOf(reLettres));
                m.insert(1,esprit);
            }
            if (m == mot)
            {
                ligne.prepend("<span style='color:orangered'>");
                ligne.append("</span>");
            }
        }
        ligne.append("<ul>");
        for (int i = 1; i<ecl.size();i++)
        {
            QStringList e = ecl[i].split("\t");
            e[1].remove("}");
            QString forme = beta2unicode(e[0],beta);
            forme.replace("σ,","ς,");
            QString lem = e[0];
            QString ff = forme;
            if (forme.contains("-"))
            {
                // Si l'indication contient un "-",
                // on trouve le lemme à sa droite
                // et le (ou les) préverbe(s) à sa gauche,
                // éventuellement séparés par des virgules.
                // Mais il y a aussi des cas où la forme avec détails est donnée !
                // 19 préverbes avec 97 141 occurrences
                // 14 125 formes avec 22 401 occurrences
                if (forme.contains(",") &&
                        ((forme.section(",",0,0).size() > 6) || (forme.section(",",0,0) == "kassw=")))
                {
                    // kassw= est le seule forme de six caractères ou moins
                    // qui ne soit pas un préfixe.
                    ff = forme.section(",",1); // forme est en grec.
                    lem = lem.section(",",1); // lem est en betacode.
                    forme = forme.section(",",0,0) + ", "; // C'est la forme avec détails
                }
                else forme = "";
                QStringList pre = lem.section("-",0,0).split(",");
                QStringList pre_gr = ff.section("-",0,0).split(",");
                lem = lem.section("-",1); // lem est maintenant l'élément à traduire
                ff = ff.section("-",1); // et ff sa forme en grec.
                for (int i = 0; i < pre.size(); i++)
                    forme.append("<a href='http://aller/" + pre[i] + "'>" + pre_gr[i] + "</a>, ");
                forme.chop(2); // ", " en trop à la fin.
                forme.append("-<a href='http://aller/" + lem + "'>" + ff);
            }
            else if (forme.contains(","))
            {
                // Si les infos contiennent une virgule mais pas de tiret,
                // il n'y a qu'une virgule qui sépare les détails de la forme et le lemme.
//                ff = forme.section(",",0,0);
                lem = e[0].section(",",1);
//                lem.remove(",");
                forme.replace(",",", <a href='http://aller/" + lem + "'>");
            }
            else forme.prepend("<a href='http://aller/" + lem + "'>");
            QString trad = _trad[lem].section("\t",_cible,_cible);
            forme.append("</a>");
            ligne.append("<li>");
            ligne.append(forme);
            ligne.append(" <b><i>" + trad + "</i></b> " +e[1]);
            ligne.append("</li>\n");
        }
        if (ligne.startsWith("<span"))
        {
            if ((llem.size() > 0) && llem[0].startsWith("<span"))
            {
                if (ligne.contains(":red")) llem.prepend(ligne + "</ul>");
                else llem.insert(1,ligne + "</ul>");
                // Je peux avoir une égalité stricte et une moyennant la majuscule
            }
            else llem.prepend(ligne + "</ul>");
        }
        else llem << ligne + "</ul>";
        // Si j'ai égalité stricte, je mets la ligne en début de liste.
        // Sinon, à la fin.
    }
    return llem;
}

/**
 * @brief Lemmatise une forme pour préparer un CSV
 * @param f : la forme à lemmatiser
 * @param beta : pour avoir des beta intérieurs dans les formes
 * @return Une QStringList avec les différents lemmes possibles
 *
 * Il s'agit de reprendre la routine Lemmat::lemmatise,
 * mais on ne veut garder que les lemmes pour les mettre
 * dans un fichier csv délimité par des tab.
 * Il n'y a donc plus de balise HTML, mais je dois distinguer
 * une forme exacte d'une forme approchée.
 */
QStringList Lemmat::lem2csv(QString f,bool beta)
{
    QString f_gr = "";
    if (!f.contains(reLettres))
    {
        f_gr = uni2betacode(f); // Je garde la forme grecque en BetaCode.
        f = nettoie2(f_gr);
//        f_gr = uni2betacode(f_gr);
        // Je préfère convertir la forme grecque en betacode pour éviter le pb tonos / oxia.
        // Mark de Wilde m'a signalé que l'accent grave remplace souvent l'accent aigu.
        // De fait, il n'y a que 18 accents graves dans analyses_gr, tjs sur les lemmes.
        f_gr.replace("\\","/");
        // Il peut y avoir un accent d'enclitique : aigu sur la dernière syllabe.
        if ((f_gr.count('/') == 2) ||
                ((f_gr.count('/') == 1) && (f_gr.count('=') == 1) && (f_gr.indexOf('/') > f_gr.indexOf('='))))
            f_gr.remove(f_gr.lastIndexOf('/'),1);
    }
//    qDebug() << beta << f << f_gr;
    QStringList llem;
    if (!_formes.contains(f))
    {
//      Si la forme n'est pas connue, je retourne une liste vide.
        return llem;
    }
    QString analyses = _formes.value(f);
    // Dans la QMap _formes, toutes les formes décorées différemment
    // sont rangées dans une même QString.
    // Elles partagent la même clef qui est la forme sans décoration.
    foreach (QString ligne, analyses.split("}\t"))
    {
        // La ligne est ici l'information liée à l'une des formes rattachées à la clef.
        QStringList ecl = ligne.split("{");
        // la ligne est composée comme suit :
        // "forme1\t{lemme1_1\tanalyse1_1}{lemme1_2\tanalyse1_2}...
        // Le premier éclat a donc un contenu différent de celui des autres.
        QString mot = ecl[0].mid(0,ecl[0].size()-1);
        // Je supprime la tabulation qui trainait.
        ligne = beta2unicode(mot,beta) + "\t";
        // Je prépare à nouveau ma ligne de réponse avec la forme en grec.
        // L'idée étant que si je n'ai pas de forme exacte,
        // je veux donner la forme approchée à côté du lemme.
//        ligne = "";
        if (mot == f_gr) ligne.prepend("<<");
        else if (f_gr.contains("+") && !mot.contains("+"))
        {
           // J'ai un tréma dans le texte et pas dans la forme analysée.
            QString b = f_gr;
            b.remove("+");
            if (mot == b) ligne.prepend("<<");
        }
        // Le mot est exactement le bon (avec toutes ses décorations).
        else if (f_gr.startsWith("*") && !mot.startsWith("*"))
        {
            // La forme du texte commence avec une majuscule (en début de phrase ?).
            QString m = f_gr.mid(1);
            if (m.indexOf(reLettres) > 0)
            {
                QString esprit = m.mid(0,m.indexOf(reLettres));
                m = m.mid(m.indexOf(reLettres));
                m.insert(1,esprit);
            }
            if (m == mot) ligne.prepend("<");
            // Le mot est bon à la majuscule prêt.
        }
        for (int i = 1; i<ecl.size();i++)
        {
            QStringList e = ecl[i].split("\t");
            e[1].remove("}");
//            QString forme = beta2unicode(e[0],beta);
            QString lem = e[0];
/*            if (lem.contains(","))
            {
                lem = e[0].section(",",1);
                lem.remove(",");
            } */
//            lem = beta2unicode(lem,beta);
            // Il ne me semble pas utile de faire la transformation ici.
            // S'il n'y a pas de forme exacte, le même lemme peut apparaître plusieurs fois.
            if (!ligne.contains(lem))
            {
                ligne.append(lem);
                ligne.append("\t");
                // Je n'ajoute le lemme que s'il n'y est pas déjà.
            }
        }
        ligne.chop(1); // Supprime le dernier tab
        if (ligne.startsWith("<"))
        {
            // Ma ligne est exacte au moins à la majuscule près.
            if ((llem.size() > 0) && llem[0].startsWith("<"))
            {
                if (ligne.startsWith("<<")) llem.prepend(ligne);
                else llem.insert(1,ligne);
                // Je peux avoir une égalité stricte et une moyennant la majuscule
            }
            else llem.prepend(ligne);
        }
        else llem << ligne;
    }
    return llem;
}

/**
 * @brief convertit une forme de betacode en unicode
 * @param f : la forme en betacode à convertir
 * @param beta : booléen pour distinguer les deux bêtas
 * @return la forme convertie en unicode
 *
 * La forme @a f est ici un mot isolé.
 * S'il se termine par un "s", c'est un sigma final "ς"
 * qu'il faudra mettre à la fin du mot en caractères grecs.
 *
 * Si le booléen @a beta est @c true, on distinguera le bêta
 * initial "β" du bêta intérieur "ϐ".
 * Sinon, tous les bêtas seront écrits "β".
 * La distinctions des bêtas semble être une tradition française.
 */
QString Lemmat::beta2unicode(QString f, bool beta)
{
    if (f.isEmpty()) return f;
    // Transf le betacode en unicode
    if (f.endsWith("s"))
    {
        f.chop(1);
        f.append("ς");
        // Le sigma final
    }
//    qDebug() << f;
    if (!beta)
    {
        f.replace("*b","Β");
        f.replace("b","β");
    }
    else if (f.startsWith("b"))
        f = "β" + f.mid(1);
//    qDebug() << beta << f;
    if ((f.size() > 1) && f[f.size()-1].isDigit() && (f[f.size()-2]=='s'))
    {
        QChar nn = f[f.size()-1];
        f.chop(2);
        f.append("ς");
        // Le sigma final
        f.append(nn);
    }
    for (int i=0; i<_beta.size();i++)
        f.replace(_beta[i],_uni[i]);
//    qDebug() << beta << f;
    return f;
}

/**
 * @brief convertit une forme de l'unicode en betacode
 * @param f : la forme en caractères grecs (unicode) à convertir
 * @return la forme convertie en betacode
 */
QString Lemmat::uni2betacode(QString f)
{
    // Transf l'unicode en betacode
    for (int i=0; i<_beta.size();i++)
        f.replace(_uni[i],_beta[i]);
//    for (int i=0; i<f.size(); i++)
  //      if (f[i].unicode() > 127) qDebug() << f[i] << f;
    return f;
}

/**
 * @brief retire d'une forme ses signes diacritiques
 * @param f : la forme en caractères grecs
 * @return la forme en betacode sans signe diacritique, ni majuscule
 */
QString Lemmat::nettoie(QString f)
{
    f.remove("\u1FBF");
    // Dans le Bailly, il y a un rho majuscule avec un esprit doux qui n'existe pas en Unicode
    // d'où l'utilisation de ce caractère "psili" tout seul (dans deux mots).
    f.remove("\u0384");
    // Dans le LSJ, il y a une entrée ΄κτώ qui utilise le "Greek tonos" comme apostrophe.
    f.remove("\u2020");
    // Dans le LSJ et dans le Pape, il y a des entrées qui commence avec un "dagger".
    QString res = uni2betacode(f);
    // $betasignes = array("(", ")", "\\", "/", "=", "+", "|", "_", "^", "*");
    res.remove("(");
    res.remove(")");
    res.remove("\\");
    res.remove("/");
    res.remove("=");
    res.remove("+");
    res.remove("|");
    res.remove("_");
    res.remove("^");
    res.remove("*");
    return res;
}

/**
 * @brief retire d'une forme ses signes diacritiques
 * @param f : la forme en betacode
 * @return la forme en betacode sans signe diacritique
 */
QString Lemmat::nettoie2(QString res)
{
    // La même que nettoie mais la forme est déjà en betacode.
//    QString res = uni2betacode(f);
    // $betasignes = array("(", ")", "\\", "/", "=", "+", "|", "_", "^", "*");
    res.remove("(");
    res.remove(")");
    res.remove("\\");
    res.remove("/");
    res.remove("=");
    res.remove("+");
    res.remove("|");
    res.remove("_");
    res.remove("^");
    res.remove("*");
    return res;
}

/**
 * @brief transformation des analyses
 * @param nom
 *
 * @deprecated n'est pas utilisé (et vide de surcroît).
 */
void Lemmat::majAnalyses(QString nom)
{
    // Contrairement aux mises à jour des dicos, où j'ai déjà recopié
    // le fichier d'origine, je veux le transformer.
    // Le fichier d'origine est humainement lisible, pour pouvoir être corrigé.
    // Toutefois, ramener l'analyse morphologique à un nombre doit permettre
    // de réduire la taille du fichier considérablement.

    // Première étape : lire le fichier et compter le nombre
    // d'occurrences de chaque morpho.

    // Deuxième étape : ranger les morphos en fonction du nb d'occurrences,
    // les mettre en liste, les numéroter et les écrire dans le fichier.

    // Troisième étape : remplacer la morpho humainement lisible par son numéro
    // et écrire le fichier de sortie.
/*    QString nomCourt = nomFichier.section("/",-1);

    if (QFile::exists(_rscrDir + "ind_gr_ana.csv"))
    {
        QFile::remove(_rscrDir + "ind_gr_ana.bak");
        QFile::rename(_rscrDir + "ind_gr_ana.csv",_rscrDir + "ind_gr_ana.bak");
    }
    // Si l'index existait, je le renomme en .bak
    if (QFile::exists(_rscrDir + nomCourt))
        QFile::remove(_rscrDir + nomCourt);
    QFile::copy(nomFichier,_rscrDir + nomCourt);
    // Je copie le fichier dans les ressources.

    // Pour le fichier "analyses_gr.txt"
    _formes.clear();
    // J'efface la QMultiMap pour la repeupler
    QFile fEntree (nom);
    if (fEntree.open (QFile::ReadOnly | QFile::Text))
    {
        QString linea;
        QString cle;
        qint64 p;
        fEntree.seek (0);
        while (!fEntree.atEnd ())
        {
            p = fEntree.pos ();
            linea = fEntree.readLine ();
            // Pour greek-analyses, c'est le premier champ avant un Tab.
            if (linea.startsWith('!')) continue;
            linea=linea.trimmed();

            cle = linea.section(char(9),0,0);
            cle.remove("(");
            cle.remove(")");
            cle.remove("/");
            cle.remove("\\");
            cle.remove("|");
            cle.remove("=");
            cle.remove("+");
            cle.remove("*");
            // j'enlève les caractères spéciaux du betacode.
            _formes.insert(cle,p);

        }
    }
    else qDebug() << "erreur";
    fEntree.close ();
    QStringList tous = _formes.keys();
    tous.removeDuplicates();
    qDebug() << _formes.size() << tous.size();
    QFile findex (_rscrDir + "ind_gr_ana.csv");
    if (findex.open (QFile::WriteOnly | QFile::Text))
    {
    QTextStream fli (&findex);
    fli.setCodec ("UTF-8");

    foreach (QString element, tous)
        foreach (qint64 p, _formes.values(element))
            fli << element << ":" << p << "\n";
        {

        }
    findex.close ();
    }

    // J'essaie de créer un nouveau fichier d'analyses qui regroupe sur une même ligne
    // toutes les formes qui ne diffèrent que par la décoration.
    QFile fGrAn (_rscrDir + "greek-analyses.txt");
    fGrAn.open (QIODevice::ReadOnly|QIODevice::Text);
    QMap<QString,QString> trad;
    QMap<QString,int> morpho;
    findex.setFileName(_rscrDir + "analyses_gr.txt");
    if (findex.open(QFile::WriteOnly | QFile::Text))
    {
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        QString format = "{%1\t%2}";
        foreach (QString element, tous)
        {
            QString linea = "";
            foreach (qint64 p, _formes.values(element))
            {
                fGrAn.seek(p);
                QString ligne = fGrAn.readLine();
                QStringList ecl = ligne.split("{");
                linea.append("\t" + ecl[0]);
                for (int i = 1; i<ecl.size();i++)
                {
                    QStringList e = ecl[i].split("\t");
                    e[2] = e[2].mid(0,e[2].indexOf("}")); // Je coupe ce qui suit la dernière }
                    QString forme = e[0].section(" ",2,2);
                    QString lem = forme;
                    if (forme.contains(","))
                    {
                        lem = forme.section(",",1);
                        lem.remove(",");
                    }
                    linea.append(format.arg(forme).arg(e[2]));
                    if (trad.contains(lem))
                    {
                        if (trad[lem] != e[1]) qDebug() << ligne << forme << lem << trad[lem];
                    }
                    else trad.insert(lem,e[1]);
                    morpho[e[2]] += 1;
                }
            }
            fli << linea.mid(1) << "\n";
            // Je sors la ligne sans le premier tab.
        }
        findex.close ();
    }
    findex.setFileName(_rscrDir + "morphos_gr.csv");
    if (findex.open(QFile::WriteOnly | QFile::Text))
    {
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        QString format = "%1\t%2\n";
        foreach (QString element, morpho.keys())
            fli << format.arg(element).arg(morpho[element]);
        findex.close();
    }
    findex.setFileName(_rscrDir + "trad_gr_en.csv");
    if (findex.open(QFile::WriteOnly | QFile::Text))
    {
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        QString format = "%1\t%2\n";
        foreach (QString element, trad.keys())
            fli << format.arg(element).arg(trad[element]);
        findex.close();
    }
    */
}

/**
 * @brief mise à jour de l'Abrégé du Bailly
 * @param nom : le chemin complet du nouveau fichier
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine fait suite à
 * MainWindow::majAB qui a recopié la copie de travail au bon endroit et
 * elle va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
void Lemmat::majAbrBailly(QString nom)
{
    // Pour le dico "XMLBailly*.txt"
    QFile fandr (_rscrDir + nom);
    QFile findex (_rscrDir + "AbrBailly.csv");
    QString cle_prec="ὤκιμον"; // dernier mot du AbrBailly
    if (fandr.open (QFile::ReadOnly | QFile::Text))
    {
        QString linea;
        QString cle;
        findex.open(QFile::WriteOnly | QFile::Text);
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        fli << "! " << fandr.fileName().section("/",-1) << "\n!\n";
        fli << QString("! Fichier d'index pour le dico sus-nommé\n");
        fli << QString("! créé par et pour la version résidente d'Eulexis.\n!\n");
        fli << QString("! Le fichier HTML de l'abrégé du Bailly est de Chaerephon.\n!\n");
        fli << "! Philippe Verkerk 2020.\n!\n";
        fli << QString("! Les lignes commençant par un ! sont des commentaires,\n!");
        qint64 p;
        fandr.seek (0);
        //    QStringList tous;
        while (!fandr.atEnd ())
        {
            // flux.flush ();
            p = fandr.pos ();
            linea = fandr.readLine ();
            if (linea.startsWith('!') || linea.isEmpty()) continue;
            linea=linea.trimmed();
            verif(linea);
            // Pour le AbrBailly, le premier mot avant un double blanc
//            cle = linea.section("  ",0,0).trimmed();
            cle = linea.section("\t",0,0).trimmed();
            cle.replace("ϐ","β"); // Le AbrBailly est le seul à utiliser le ϐ intérieur.
            QString clef = nettoie(cle).toLower();
            // La clef est la version, en caractères latins sans décoration, de cle (en grec avec déco).
            if (clef.contains("-"))
            {
                if (clef.startsWith("-")) clef = clef.mid(1).trimmed();
                else clef = clef.section("-",0,0).trimmed();
            }
            if (clef.contains("–"))
            {
                if (clef.startsWith("–")) clef = clef.mid(1).trimmed();
                else clef = clef.section("–",0,0).trimmed();
            }
            if (clef.contains(" ")) clef = clef.section(" ",0,0);
            if (clef.contains(",")) clef = clef.section(",",0,0);
            clef.remove("¹");
            clef.remove("²");
            clef.remove("³");
            clef.remove("⁴");
            clef.remove("⁵");

            fli << cle << "\n" << clef << ":" << p << ":" << cle << ":" << cle_prec << ":";
            // La ligne est incomplète : la clé suivante viendra la compléter.
            cle_prec = cle;
        }
        fli << QString("Α\n"); // Compléter la dernière ligne avec le premier mot.
    }
    else qDebug() << "erreur";
    fandr.close ();
    findex.close ();
    lireAbrBailly();
}

/**
 * @brief mise à jour du Bailly
 * @param nom : le chemin complet du nouveau fichier
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine fait suite à
 * MainWindow::majB qui a recopié la copie de travail au bon endroit et
 * elle va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
void Lemmat::majBailly(QString nom)
{
    // Pour le dico "Bailly2020.txt"
    QString lgFormat = ":%1:%2:%3:";
    QString lg;
    QStringList listClefs;
    QFile fandr (_rscrDir + nom);
    QFile findex (_rscrDir + "Bailly.csv");
    QString cle_prec="ᾠῶν"; // dernier mot du Bailly 2020
    if (fandr.open (QFile::ReadOnly | QFile::Text))
    {
        QString linea;
        QString cle;
        findex.open(QFile::WriteOnly | QFile::Text);
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        fli << "! " << fandr.fileName().section("/",-1) << "\n!\n";
        fli << QString("! Fichier d'index pour le dico sus-nommé\n");
        fli << QString("! créé par et pour la version résidente d'Eulexis.\n!\n");
        fli << QString("! Le Bailly 2020 a été mis en TeX par Gérard Gréco et son équipe.\n");
        fli << "! Il est disponible ici avec l'autorisation de l'auteur.\n!\n";
        fli << "! Philippe Verkerk 2020.\n!\n";
        fli << QString("! Les lignes commençant par un ! sont des commentaires,\n!\n");
        qint64 p;
        fandr.seek (0);
        //    QStringList tous;
        while (!fandr.atEnd ())
        {
            // flux.flush ();
            p = fandr.pos ();
            linea = fandr.readLine ();
            if (linea.startsWith('!') || linea.isEmpty() || !linea.startsWith("<div")) continue;
            linea=linea.trimmed();
            verif(linea);
            // Pour le Bailly, la cle et les entrées secondaires sont dans le <div...>
            cle = linea.section("'",1,1).trimmed();
            // Pour l'instant, je ne garde que la clef principale.
            cle.replace("ϐ","β"); // Le Bailly est le seul à utiliser le ϐ intérieur.
            if (!listClefs.isEmpty())
            {
                // Je dois ajouter la nouvelle cle à la fin des lignes
                for (int i = 0; i < listClefs.size(); i++)
                    fli << listClefs[i] << lg << cle << "\n";
                listClefs.clear();
            }
            lg = lgFormat.arg(p).arg(cle).arg(cle_prec);
            cle.replace("\u00A0"," ");
            cle_prec = cle;
            QString clef = nettoie(cle).toLower();
            if ((clef.size() > 1) && clef[clef.size()-1].isDigit()) clef.chop(1);
            // La clef est la version, en caractères latins sans décoration, de cle (en grec avec déco).
            if (clef.contains("-"))
            {
                if (clef.startsWith("-")) clef = clef.mid(1).trimmed();
                else clef = clef.section("-",0,0).trimmed();
            }
            if (clef.contains("–"))
            {
                if (clef.startsWith("–")) clef = clef.mid(1).trimmed();
                else clef = clef.section("–",0,0).trimmed();
            }
            if (clef.contains(" ")) clef = clef.section(" ",0,0);
            if (clef.contains(",")) clef = clef.section(",",0,0);

            listClefs.append(clef);
            if (linea.contains("var='"))
            {
                // J'ai des entrées secondaires...
                if (cle.contains("-")) qDebug() << linea;
                cle = linea.section("'>",0,0);
                cle = cle.section("'",-1);
                cle = nettoie(cle).toLower();
                listClefs.append(cle.split(", "));
                listClefs.removeDuplicates();
            }

//            fli << cle << "\n" << clef << ":" << p << ":" << cle << ":" << cle_prec << ":";
            // La ligne est incomplète : la clé suivante viendra la compléter.
        }
        if (!listClefs.isEmpty())
        {
            // J'ai fini le fichier, mais pas sorti la dernière ligne.
            for (int i = 0; i < listClefs.size(); i++)
                fli << listClefs[i] << lg << QString("Α\n");
            // Je complète la dernière ligne avec le premier mot qui est un alpha majuscule.
            // Le QString est là pour assurer que la transcription se fera bien en utf-8.
            listClefs.clear();
        }
    }
    else qDebug() << "erreur";
    fandr.close ();
    findex.close ();
    lireBailly();
}

/**
 * @brief mise à jour du LSJ
 * @param nom : le chemin complet du nouveau fichier
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine fait suite à
 * MainWindow::majL qui a recopié la copie de travail au bon endroit et
 * elle va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
void Lemmat::majLSJ(QString nom)
{
    QString linea;
    QString cle;
/*    // Chaeréphon a modifié le format des références dans le LSJ.
    // Je dois donc chercher sous quelle forme il les a mises.
    QFile fRef(_rscrDir + "Liste_Ref_LSJ.csv");
    // C'est la liste telle que je l'avais constituée.
    QStringList lignes;
    QStringList formes;
    QList<int> occ;
    if (fRef.open (QFile::ReadOnly | QFile::Text))
    {
        while (!fRef.atEnd())
        {
            linea = fRef.readLine();
            cle = linea.section("\t",0,0);
            lignes << linea;
            formes << cle;
            occ << 0;
            // Je mets la forme d'origine de la référence.
            // Si elle contenait des espaces, il est probable qu'elle ait changé.
            int p=0;
            if (cle.contains(" "))
            {
                QString c = cle;
                c.remove(" ");
                lignes << linea;
                formes << c;
                occ << 0;
                for (int i=0; i<cle.count(" "); i++)
                {
                    // J'essaie différentes formes.
                    c = cle;
                    int q=c.indexOf(" ",p+1);
                    c.insert(q+1,"<i>");
                    c.append("</i>");
                    lignes << linea;
                    formes << c;
                    occ << 0;
                    if (p!=0)
                    {
                        c.remove(p,1);
                        lignes << linea;
                        formes << c;
                        occ << 0;
                    }
                    c.remove(" ");
                    c.replace("<i>", " <i>");
                    if (c != formes.last())
                    {
                        lignes << linea;
                        formes << c;
                        occ << 0;
                    }
                    p = q;
                }
            }
        }
        fRef.close();
        qDebug() << lignes.size() << formes.size() << occ.size();
    }
*/
// Pour le dico "LSJ*.txt"
    QFile fandr (_rscrDir + nom);
    QFile findex (_rscrDir + "LSJ.csv");
    QString cle_prec="ᾠώδης"; // dernier mot du LSJ
    if (fandr.open (QFile::ReadOnly | QFile::Text))
    {
        findex.open(QFile::WriteOnly | QFile::Text);
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        fli << "! " << fandr.fileName().section("/",-1) << "\n!\n";
        fli << QString("! Fichier d'index pour le dico sus-nommé\n");
        fli << QString("! créé par et pour la version résidente d'Eulexis.\n!\n");
        fli << QString("! Le fichier HTML du LSJ a été revu Chaerephon.\n!\n");
        fli << "! Philippe Verkerk 2020.\n!\n";
        fli << QString("! Les lignes commençant par un ! sont des commentaires,\n!");
        qint64 p;
        fandr.seek (0);
        //    QStringList tous;
        while (!fandr.atEnd ())
        {
            // flux.flush ();
            p = fandr.pos ();
            linea = fandr.readLine ();
            if (linea.startsWith('!') || linea.isEmpty()) continue;
            linea=linea.trimmed();
            verif(linea);
            // Pour compter les références.
//            for (int i=0; i<formes.size(); i++)
  //              occ[i] += linea.count(formes[i]);
            // Pour le LSJ, le premier mot avant un tab
            cle = linea.section(char(9),0,0).trimmed();
            if (cle.contains(reLettres)) qDebug() << cle << linea;
            QString clef = nettoie(cle).toLower();
            // La clef est la version, en caractères latins sans décoration, de cle (en grec avec déco).
            if (clef.contains("-"))
            {
                if (clef.startsWith("-")) clef = clef.mid(1).trimmed();
                else clef = clef.section("-",0,0).trimmed();
            }
            if (clef.contains("–"))
            {
                if (clef.startsWith("–")) clef = clef.mid(1).trimmed();
                else clef = clef.section("–",0,0).trimmed();
            }
            if (clef.contains(" ")) clef = clef.section(" ",0,0);
            if (clef.contains(",")) clef = clef.section(",",0,0);
            clef.remove("¹");
            clef.remove("²");
            clef.remove("³");
            clef.remove("⁴");
            clef.remove("⁵");

            fli << cle << "\n" << clef << ":" << p << ":" << cle << ":" << cle_prec << ":";
            // La ligne est incomplète : la clé suivante viendra la compléter.
            cle_prec = cle;
        }
        fli << QString("Α\n"); // Compléter la dernière ligne avec le premier mot.
    }
    else qDebug() << "erreur";
    fandr.close ();
    findex.close ();
    lireLSJ();
/*
    // Sortir le dénombrement des ref.
    QString format = "%1\t%2\t%3";
    fRef.setFileName(_rscrDir + "cnt_Ref_LSJ.csv");
    if (fRef.open (QFile::WriteOnly | QFile::Text))
        for (int i=0; i<formes.size(); i++)
        {
            linea = format.arg(formes[i]).arg(occ[i]).arg(lignes[i]);
            fRef.write(linea.toUtf8());
        }
    fRef.close();
*/
}

/**
 * @brief Charge en mémoire l'index du LSJ
 */
void Lemmat::lireLSJ()
{
    // Charger en mémoire l'index du LSJ
    if (!QFile::exists(_rscrDir + "LSJ.csv")) return;
    _LSJindex.clear();
    QFile findex (_rscrDir + "LSJ.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString linea = findex.readLine();
    _LSJname = linea.mid(1).trimmed();
    if (_LSJname.isEmpty()) qDebug() << "Erreur : le nom du LSJ manque";
//    else qDebug() << _LSJname;

    int i = findex.size()/100;
    int ratio = 1024;
    while (ratio < i) ratio *= 2;
    QProgressDialog progr("Chargement du LSJ...", QString(), 0, findex.size()/ratio);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    i=0;
    int j=0;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        i += linea.size();
        if (j != i/ratio)
        {
            j = i / ratio;
            progr.setValue(j);
        }
        if (linea.startsWith('!') || linea.isEmpty()) continue;
        if (linea.contains(":"))
        {
            // Tout va bien !
            if (linea.endsWith("\n"))
                linea.chop(1);
            _LSJindex.insert(linea.section(":",0,0),linea);
        }
        else qDebug() << "Erreur dans l'index du LSJ : " << linea;
    }
    findex.close ();
//    qDebug() << _LSJindex.size();

    // Je lis aussi la liste des références aux œuvres.
    findex.setFileName(_rscrDir + "Ref_LSJ.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString f = "@%1@";
    // $ref_apres[$x_ref] = "<a href='Liste_Auteurs_LSJ.htm#" . $eclats[1] . "' title='" . $eclats[2] . "'>" . $eclats[0] . "</a>";
    QString href = " <a href='@%1'>%2</a>";
    // Pour que les références les plus longues passent d'abord !
//    QMultiMap<int,QString> ordre;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        QStringList eclats = linea.split("\t");
        if (eclats[0] != "AD")
        {
            // AD est Apollonius Dyscolus, mais surtout Anno Domini !
            eclats[4].replace("'","’");
            eclats[4].replace("<","≤");
            eclats[4].replace(">","≥");
            _refLSJ << " " + eclats[0];
            _tmpLSJ << f.arg(_refLSJ.size()-1);
            _renLSJ << href.arg(eclats[4]).arg(eclats[0]);
        }
//        qDebug() << _tmpLSJ.last() << _refLSJ.last() << _renLSJ.last();
//        ordre.insert(-eclats[0].size(),linea);
    }
    findex.close ();
    /*
    findex.setFileName(_rscrDir + "Ref_LSJ2.csv");
    findex.open(QFile::WriteOnly | QFile::Text);
    QList<int> clefs = ordre.keys();
    for (int i=clefs.size()-1; i>0; i--)
        if (clefs[i-1] == clefs[i]) clefs.removeAt(i);
    foreach (int i, clefs) {
        foreach (linea, ordre.values(i)) {
            findex.write(linea.toUtf8());
        }
    }
    findex.close ();
    */
}

/**
 * @brief Charge en mémoire l'index de l'abrégé du Bailly
 */
void Lemmat::lireAbrBailly()
{
    // Charger en mémoire l'index du Bailly
    if (!QFile::exists(_rscrDir + "AbrBailly.csv")) return;
    _AbrBaillyIndex.clear();
    QFile findex (_rscrDir + "AbrBailly.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString linea = findex.readLine();
    _AbrBaillyName = linea.mid(1).trimmed();
    if (_AbrBaillyName.isEmpty()) qDebug() << "Erreur : le nom du Bailly manque";
//    else qDebug() << _AbrBaillyName;

    int i = findex.size()/100;
    int ratio = 1024;
    while (ratio < i) ratio *= 2;
    QProgressDialog progr("Chargement de l'abrégé du Bailly...", QString(), 0, findex.size()/ratio);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    i=0;
    int j=0;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        i += linea.size();
        if (j != i/ratio)
        {
            j = i / ratio;
            progr.setValue(j);
        }
        if (linea.startsWith('!') || linea.isEmpty()) continue;
        if (linea.contains(":"))
        {
            // Tout va bien !
            if (linea.endsWith("\n"))
                linea.chop(1);
            _AbrBaillyIndex.insert(linea.section(":",0,0),linea);
        }
        else qDebug() << "Erreur dans l'index du Bailly : " << linea;
    }
    findex.close ();
//    qDebug() << _AbrBaillyIndex.size();
}

/**
 * @brief Charge en mémoire l'index du Bailly
 */
void Lemmat::lireBailly()
{
    QMap <QString,int> occ;
    // Charger en mémoire l'index du Bailly
    if (!QFile::exists(_rscrDir + "Bailly.csv")) return;
    _BaillyIndex.clear();
    QFile findex (_rscrDir + "Bailly.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString linea = findex.readLine();
    _BaillyName = linea.mid(1).trimmed();
    if (_BaillyName.isEmpty()) qDebug() << "Erreur : le nom du Bailly manque";
//    else qDebug() << _BaillyName;

    int i = findex.size()/100;
    int ratio = 1024;
    while (ratio < i) ratio *= 2;
    QProgressDialog progr("Chargement du Bailly...", QString(), 0, findex.size()/ratio);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    i=0;
    int j=0;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        i += linea.size();
        if (j != i/ratio)
        {
            j = i / ratio;
            progr.setValue(j);
        }
        if (linea.startsWith('!') || linea.isEmpty()) continue;
        if (linea.contains(":"))
        {
            // Tout va bien !
            if (linea.endsWith("\n"))
                linea.chop(1);
            _BaillyIndex.insert(linea.section(":",0,0),linea);
        }
        else qDebug() << "Erreur dans l'index du Bailly : " << linea;
    }
    findex.close ();
//    qDebug() << _BaillyIndex.size();
/*    QStringList ll = _BaillyIndex.keys();
    ll.removeDuplicates();
    findex.setFileName("/Users/Philippe/Documents/Bailly_cnt.csv");
    findex.open(QFile::WriteOnly | QFile::Text);
    foreach (QString bla, ll)
    {
        QString b = bla + ":%1\n";
        b = b.arg(_BaillyIndex.values(bla).size());
        findex.write(b.toUtf8());
    }
    findex.close(); */
}

/**
 * @brief Charge en mémoire l'index du Pape
 */
void Lemmat::lirePape()
{
    // Charger en mémoire l'index du Pape
    if (!QFile::exists(_rscrDir + "Pape.csv")) return;
    _PapeIndex.clear();
    QFile findex (_rscrDir + "Pape.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString linea = findex.readLine();
    _PapeName = linea.mid(1).trimmed();
    if (_PapeName.isEmpty()) qDebug() << "Erreur : le nom du Pape manque";
//    else qDebug() << _PapeName;

    int i = findex.size()/100;
    int ratio = 1024;
    while (ratio < i) ratio *= 2;
    QProgressDialog progr("Chargement du Pape...", QString(), 0, findex.size()/ratio);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    i=0;
    int j=0;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        i += linea.size();
        if (j != i/ratio)
        {
            j = i / ratio;
            progr.setValue(j);
        }
        if (linea.startsWith('!') || linea.isEmpty()) continue;
        if (linea.contains(":"))
        {
            // Tout va bien !
            if (linea.endsWith("\n"))
                linea.chop(1);
            _PapeIndex.insert(linea.section(":",0,0),linea);
        }
        else qDebug() << "Erreur dans l'index du Pape : " << linea;
    }
    findex.close ();
//    qDebug() << _PapeIndex.size();
}

/**
 * @brief consultation d'un dictionnaire par une expression rationnelle
 * @param f : l'expression rationnelle à chercher
 * @param dicIndex : un pointeur vers l'index du dictionnaire
 * @return la liste des entrées de l'index qui coïncident avec l'expression rationnelle.
 */
QStringList Lemmat::consRegExp(QString f, QMultiMap<QString, QString> *dicIndex)
{
    QStringList llem;
    QRegExp re = QRegExp(f); // Je suppose que f est une expression rationnelle.
    if (!re.isValid()) return llem;
    QStringList lEntrees = dicIndex->keys();
    lEntrees.removeDuplicates();
    for (int i = lEntrees.size() - 1; i > -1; i--)
    {
        if (re.exactMatch(lEntrees[i]) && (llem.size() < _maxList))
            llem.append(dicIndex->values(lEntrees[i]));
    }
    return llem;
}

/**
 * @brief consultation d'un dictionnaire avec des caractères de substitution
 * @param f : la forme à chercher avec ses caractères de substitution ("*" et "?")
 * @param dicIndex : un pointeur vers l'index du dictionnaire
 * @return la liste des entrées de l'index qui coïncident avec la forme demandée.
 */
QStringList Lemmat::consAsterisk(QString f, QMultiMap<QString, QString> *dicIndex)
{
    QStringList llem;
    QStringList lEntrees = dicIndex->keys();
    lEntrees.removeDuplicates();
    int longueur = f.size();
    bool interrog = f.contains("?");
    f.replace("**","*"); // Des cas idiots qu'il vaut mieux éliminer.
    f.replace("?*","*");
    f.replace("*?","*");
    if (((f.count("*") == 1) && (f.count("?") == 0)) ||
            ((f.count("*") == 0) && ((f.count("?") == 1) || (f.count("??") == 1))))
    {
        // Trois cas possibles bla*, *bla ou bla*bla
        if (f.startsWith("*") || f.startsWith("?") ||
                (f[0].isDigit() && ((f[1] == '?') || (f[1] == '*'))))
        {
            // Je dois examiner toute la liste
            if (f[0].isDigit())
            {
                longueur += f.mid(0,1).toInt() - 2;
                f = f.mid(2);
            }
            else
            {
                if (f[0] == '*') longueur = 1000; // Pas de limite.
                f = f.mid(1);
            }
            if (f.startsWith("?")) f = f.mid(1); // Double ? au début
            for (int i = lEntrees.size() - 1; i > -1; i--)
                if (lEntrees[i].endsWith(f) && (llem.size() < _maxList) &&
                        ((interrog && (longueur == lEntrees[i].size())) ||
                         (!interrog && (longueur >= lEntrees[i].size()))))
                    llem.append(dicIndex->values(lEntrees[i]));
        }
        else if (f.endsWith("*") || f.endsWith("?"))
        {
            if (f[f.size() - 2].isDigit())
            {
                longueur += f.mid(f.size() - 2,1).toInt() - 2;
                f.chop(2);
            }
            else
            {
                if (f[f.size() - 1] == '*') longueur = 1000; // Pas de limite.
                f.chop(1);
            }
            if (f.endsWith("?")) f.chop(1);
            f.append("}");
            int i = lEntrees.size() - 1;
            while ((i > -1) && (lEntrees[i] > f)) i--;
            f.chop(1);
            while ((i > -1) && lEntrees[i].startsWith(f) && (llem.size() < _maxList))
            {
                if ((interrog && (longueur == lEntrees[i].size())) ||
                        (!interrog && (longueur >= lEntrees[i].size())))
                    llem.append(dicIndex->values(lEntrees[i]));
                i--;
            }
        }
        else
        {
            // Un début et une fin
            int pp = f.indexOf("*");
            if (pp == -1) pp = f.indexOf("?");
            QString fin = f.mid(pp + 1);
            if (fin.startsWith("?")) fin = fin.mid(1); // Double ? au milieu
            if (f[pp - 1].isDigit())
            {
                longueur += f.mid(pp-1,1).toInt() - 2;
                pp--;
            }
            else if (f[pp] == '*') longueur = 1000;
            QString deb = f.mid(0,pp) + "}";
            int i = lEntrees.size() - 1;
            while ((i > -1) && (lEntrees[i] > deb)) i--;
            deb.chop(1);
            while ((i > -1) && lEntrees[i].startsWith(deb) && (llem.size() < _maxList))
            {
                QString e = lEntrees[i].mid(deb.size());
                // Il faut retirer le début avant de chercher la fin !
                // Erreur trouvée en cherchant s*sainw qui m'a donné sainw.
                if (e.endsWith(fin) &&
                        ((interrog && (longueur == lEntrees[i].size())) ||
                         (!interrog && (longueur >= lEntrees[i].size()))))
                    llem.append(dicIndex->values(lEntrees[i]));
                i--;
            }
        }
    }
    else
    {
        // Je commence par faire le découpage
        int pp1 = f.indexOf("*");
        int pp2 = f.lastIndexOf("*");
        int lim1 = 1000;
        int lim2 = 1000;
        bool ex1 = false;
        bool ex2 = false;
        if (pp1 == -1)
        {
            // Je n'avais pas d'*
            pp1 = f.indexOf("?");
            pp2 = f.lastIndexOf("?");
            lim1 = 1;
            lim2 = 1;
            ex1 = true;
            ex2 = true;
        }
        else if (pp1 == pp2)
        {
            // Il n'y a qu'une *
            int pp = f.indexOf("?");
            if (pp < pp1)
            {
                pp1 = pp;
                lim1 = 1;
                ex1 = true;
            }
            else
            {
                pp2 = pp;
                lim2 = 1;
                ex2 = true;
            }
        }
        // pp1 est la position du 1er caractère de substitution et pp2 celle du 2e.
        QString deb = f.mid(0,pp2);
        QString fin = f.mid(pp2 + 1);
        QString mil = deb.mid(pp1 + 1);
        deb = deb.mid(0,pp1);
        if ((pp1 > 0) && f[pp1 - 1].isDigit())
        {
            deb.chop(1);
            lim1 = f.mid(pp1 - 1, 1).toInt();
        }
        if (f[pp2 - 1].isDigit())
        {
            mil.chop(1);
            lim2 = f.mid(pp2 - 1, 1).toInt();
        }
        if (mil.isEmpty()) return llem;
        // Ce n'est pas une requête absurde (du genre ?2* = de 1 à 3 caractères quelconques, pas 0)
        // mais je n'ai pas envie de considérer ce cas
        if (deb.isEmpty())
        {
            if (fin.isEmpty())
            {
                // Ni début, ni fin.
                for (int i = lEntrees.size() - 1; i > -1; i--)
                    if (lEntrees[i].contains(mil) && (llem.size() < _maxList))
                    {
                        // C'est un bon début !
                        QString e = lEntrees[i];
                        bool OK = (e.size() <= lim1 + lim2 + mil.size());
                        // Si j'ai des limites, je peux éliminer les mots trop longs.
                        if (OK)
                        {
                            // Plus en détail...
                            if (ex1)
                            {
                                e = e.mid(lim1);
                                if (ex2) OK = e.startsWith(mil) && (e.size() == lim2 + mil.size());
                                else OK = e.startsWith(mil) && (e.size() <= lim2 + mil.size());
                            }
                            else if (ex2)
                            {
                                e.chop(lim2);
                                OK = e.endsWith(mil) && (e.size() <= lim1 + mil.size());
                            }
                            else if (e.size() > lim1)
                            {
                                int pp = e.mid(0,lim1 + mil.size()).indexOf(mil);
                                OK = (pp != -1) && (pp + mil.size() + lim2 >= e.size());
                            }
                            else if (e.size() > lim2)
                            {
                                OK = e.right(lim2 + mil.size()).contains(mil);
                            }
                        }
                        if (OK)
                            llem.append(dicIndex->values(lEntrees[i]));
                    }
            }
            else
            {
                // Pas de début, mais un milieu et une fin.
                for (int i = lEntrees.size() - 1; i > -1; i--)
                    if (lEntrees[i].endsWith(fin) && (llem.size() < _maxList))
                    {
                        QString e = lEntrees[i];
                        e.chop(fin.size());
                        bool OK = e.contains(mil);
                        if (OK)
                        {
                            if (ex1)
                            {
                                e = e.mid(lim1);
                                if (ex2) OK = e.startsWith(mil) && (e.size() == lim2 + mil.size());
                                else OK = e.startsWith(mil) && (e.size() <= lim2 + mil.size());
                            }
                            else if (ex2)
                            {
                                e.chop(lim2);
                                OK = e.endsWith(mil) && (e.size() <= lim1 + mil.size());
                            }
                            else if (e.size() > lim1)
                            {
                                int pp = e.mid(0,lim1 + mil.size()).indexOf(mil);
                                OK = (pp != -1) && (pp + mil.size() + lim2 >= e.size());
                            }
                            else if (e.size() > lim2)
                            {
                                OK = e.right(lim2 + mil.size()).contains(mil);
                            }
                        }
                        if (OK)
                            llem.append(dicIndex->values(lEntrees[i]));
                    }
            }
        }
        else
        {
            if (fin.isEmpty())
            {
                // Un début et un milieu, pas de fin.
                deb.append("}");
                int i = lEntrees.size() - 1;
                while ((i > -1) && (lEntrees[i] > deb)) i--;
                deb.chop(1);
                while ((i > -1) && lEntrees[i].startsWith(deb) && (llem.size() < _maxList))
                {
                    QString e = lEntrees[i].mid(deb.size());
                    bool OK = e.contains(mil);
                    if (OK)
                    {
                        // Est-ce que les contraintes sont vérifiées ?
                        if (ex1)
                        {
                            e = e.mid(lim1);
                            if (ex2) OK = e.startsWith(mil) && (e.size() == lim2 + mil.size());
                            else OK = e.startsWith(mil) && (e.size() <= lim2 + mil.size());
                        }
                        else if (ex2)
                        {
                            e.chop(lim2);
                            OK = e.endsWith(mil) && (e.size() <= lim1 + mil.size());
                        }
                        else if (e.size() > lim1)
                        {
                            int pp = e.mid(0,lim1 + mil.size()).indexOf(mil);
                            OK = (pp != -1) && (pp + mil.size() + lim2 >= e.size());
                        }
                        else if (e.size() > lim2)
                        {
                            OK = e.right(lim2 + mil.size()).contains(mil);
                        }
                    }
                    if (OK)
                        llem.append(dicIndex->values(lEntrees[i]));
                    i--;
                }
            }
            else
            {
                // Un début, un milieu et une fin.
                deb.append("}");
                int i = lEntrees.size() - 1;
                while ((i > -1) && (lEntrees[i] > deb)) i--;
                deb.chop(1);
                while ((i > -1) && lEntrees[i].startsWith(deb) && (llem.size() < _maxList))
                {
                    QString e = lEntrees[i].mid(deb.size());
                    bool OK = e.contains(mil) && e.endsWith(fin);
                    if (OK)
                    {
                        e.chop(fin.size());
                        // Est-ce que les contraintes sont vérifiées ?
                        if (ex1)
                        {
                            e = e.mid(lim1);
                            if (ex2) OK = e.startsWith(mil) && (e.size() == lim2 + mil.size());
                            else OK = e.startsWith(mil) && (e.size() <= lim2 + mil.size());
                        }
                        else if (ex2)
                        {
                            e.chop(lim2);
                            OK = e.endsWith(mil) && (e.size() <= lim1 + mil.size());
                        }
                        else if (e.size() > lim1)
                        {
                            int pp = e.mid(0,lim1 + mil.size()).indexOf(mil);
                            OK = (pp != -1) && (pp + mil.size() + lim2 >= e.size());
                        }
                        else if (e.size() > lim2)
                        {
                            OK = e.right(lim2 + mil.size()).contains(mil);
                        }
                    }
                    if (OK)
                        llem.append(dicIndex->values(lEntrees[i]));
                    i--;
                }
            }
        }
    }
    return llem;
}

/**
 * @brief cherche une forme dans l'index d'un dictionnaire
 * @param f : la forme à chercher
 * @param dicIndex : un pointeur vers l'index du dictionnaire
 * @return la liste des entrées de l'index qui coïncident avec la forme demandée.
 *
 * La forme à chercher dans l'index peut être données en caractères grecs ou latins.
 * Dans ce dernier cas (et dans ce cas seulement), la forme peut contenir
 * des expressions rationnelles **ou** des caractères de substitution.
 * * Si j'ai une forme grecque avec diacritiques, je ne fais que la recherche directe
 * et si possible je ne garde que la forme exacte.
 * * Si la forme contient une expression rationnelle, je ferai appel
 * à Lemmat::consRegExp.
 * * Si j'ai des caractères de substitution, j'appellerai Lemmat::consAsterisk.
 * * Sinon, je fais une recherche directe.
 */
QStringList Lemmat::cherchIndex(QString f, QMultiMap<QString, QString> *dicIndex)
{
    QString f_gr = "";
    if (!f.contains(reLettres))
    {
        f_gr = f; // Je garde la forme grecque.
        f = nettoie(f_gr);
    }
    if (f.contains("–"))
    {
        if (f.startsWith("–")) f = f.mid(1).trimmed();
        else f = f.section("–",0,0).trimmed();
    }
    if (f.contains(" ")) f = f.section(" ",0,0);

    QStringList llem;
    if (f_gr != "")
    {
        llem = dicIndex->values(f);
        // Si j'ai une forme grecque, je ne fais que la recherche directe.
        if (llem.isEmpty()) return llem;
        QStringList ll = llem;
        // Je dois regarder si j'ai l'entrée exacte
        for (int i = llem.size() - 1; i > -1; i--)
            if (f_gr != llem[i].section(":",2,2)) llem.removeAt(i);
        if (llem.isEmpty())
        {
            llem = ll;
            // S'il ne reste rien, je reprends tout
            if (f_gr[f_gr.size()-1].isDigit())
            {
                f_gr.chop(1);
                for (int i = llem.size() - 1; i > -1; i--)
                    if (!llem[i].section(":",2,2).startsWith(f_gr))
                        llem.removeAt(i);
                if (llem.isEmpty()) llem = ll;
            }
        }
    }
    else if (f.contains(".") || f.contains("[") || f.contains("(")
             || f.contains("|") || f.contains("{") || f.contains("\\"))
        llem = consRegExp(f, dicIndex);
    else if ((f.contains("*") || f.contains("?"))
            && (f.size() > 1 + f.count("*"))
            && (f.count("*") + f.count("?") < 3))
        llem = consAsterisk(f, dicIndex);
    else  llem = dicIndex->values(f);

    return llem;
}

/**
 * @brief consultation du LSJ
 * @param f : la forme à chercher
 * @return une liste avec les articles correspondants (en HTML)
 *
 * Cette fonction appelle la fonction générique Lemmat::consult
 * et essaie ensuite d'ajouter des liens à chaque fois qu'elle
 * pense avoir trouvé un renvoi (dont la forme dépend du dictionnaire).
 */
QStringList Lemmat::consLSJ(QString f)
{
    // Consulter le LSJ pour la forme f
    QStringList llem = cherchIndex(f, &_LSJindex);

    if (llem.isEmpty()) return llem;
//    qDebug() << llem.size() << f;
    QStringList res = consult(_LSJname,llem,"L");
    // Je fais du post-traitement : recherche des renvois.
    if (!res.isEmpty()) for (int i=3; i<res.size();i++)
    {
        // Les 3 premiers éléments donnent le mot avant, le mot après et les liens.
        QString article = res[i];
        article.replace("\t"," "); // le tab
        article.replace("\u00a0","&nbsp;"); // l'espace insécable
        if (article.contains("v. ") || article.contains(" = ") || article.contains(".</i> for "))
        {
            // Il y a probablement des renvois
            article = lierRenvois(article, " v. ");
            article = lierRenvois(article, "(v. ");
            article = lierRenvois(article, " v. sub ");
            article = lierRenvois(article, ".</i> for ");
            article = lierRenvois(article, " = ");
        }
        for (int j=0; j<_refLSJ.size(); j++)
            article.replace(_refLSJ[j],_tmpLSJ[j]);
        for (int j=0; j<_refLSJ.size(); j++)
            article.replace(_tmpLSJ[j],_renLSJ[j]);

        res[i] = article;
    }
    return res;
}

/**
 * @brief consultation de l'abrégé du Bailly
 * @param f : la forme à chercher
 * @return une liste avec les articles correspondants (en HTML)
 *
 * Cette fonction appelle la fonction générique Lemmat::consult
 * et essaie ensuite d'ajouter des liens à chaque fois qu'elle
 * pense avoir trouvé un renvoi (dont la forme dépend du dictionnaire).
 */
QStringList Lemmat::consAbrBailly(QString f)
{
    // Consulter le Bailly pour la forme f

    QStringList llem = cherchIndex(f, &_AbrBaillyIndex);

    if (llem.isEmpty()) return llem;
    QStringList res = consult(_AbrBaillyName,llem,"A");
    // Je fais du post-traitement : recherche des renvois.
    if (!res.isEmpty()) for (int i=3; i<res.size();i++)
    {
        // Les 3 premiers éléments donnent le mot avant, le mot après et les liens.
        QString article = res[i];
//        qDebug() << article;
        article.replace("\\n    ","\n&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
        article.replace("\\n\\n","<br />\n");
        article.replace("\\n","<br />\n");
        article.replace("\u00a0","&nbsp;"); // l'espace insécable
        article.replace(" ;","&nbsp;;"); // Une espace insécable avant ;
        article.replace(" :","&nbsp;:"); // Une espace insécable avant :
        article.replace(" ?","&nbsp;?"); // Une espace insécable avant ?
        article.replace(" !","&nbsp;!"); // Une espace insécable avant !
        article.replace(" »","&nbsp;»"); // Une espace insécable avant »
        article.replace("« ","«&nbsp;"); // Une espace insécable après «
//        qDebug() << article;
        int p = article.indexOf("<i><b>Moy.</b></i>");
        if (article.mid(p-1,1) != "\n") article.insert(p,"<br />\n");
        article.replace("\t"," "); // le tab
        if (article.contains("v.") || article.contains("c.") || article.contains("p."))
        {
            // Il y a probablement des renvois
//            qDebug() << article;
            article = lierRenvois(article, " v.");
            article = lierRenvois(article, " c.");
            article = lierRenvois(article, " p.");
            article = lierRenvois(article, ">p.</i> ");
            article = lierRenvois(article, ">v.</i> ");
            article = lierRenvois(article, ">c.</i> ");
            article = lierRenvois(article, ">p.</i>&nbsp;");
            article = lierRenvois(article, ">v.</i>&nbsp;");
            article = lierRenvois(article, ">c.</i>&nbsp;");
//            qDebug() << article;
        }
        res[i] = article;
    }
    return res;
}

/**
 * @brief consultation du Bailly
 * @param f : la forme à chercher
 * @return une liste avec les articles correspondants (en HTML)
 *
 * Cette fonction appelle la fonction générique Lemmat::consult
 * et essaie ensuite d'ajouter des liens à chaque fois qu'elle
 * pense avoir trouvé un renvoi (dont la forme dépend du dictionnaire).
 */
QStringList Lemmat::consBailly(QString f)
{
    // Consulter le Bailly pour la forme f

    QStringList llem = cherchIndex(f, &_BaillyIndex);

    if (llem.isEmpty()) return llem;
    QStringList res = consult(_BaillyName,llem,"B");
    // Je fais du post-traitement : recherche des renvois.
    if (!res.isEmpty()) for (int i=3; i<res.size();i++)
    {
        // Les 3 premiers éléments donnent le mot avant, le mot après et les liens.
        QString article = res[i];
//        qDebug() << article;
        article.replace("\\n    ","\n&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
        article.replace("\\n\\n","<br/>\n");
//        article.replace("'","’");
        article.replace("\u00a0","&nbsp;"); // l'espace insécable
        article.replace(" ;","&nbsp;;"); // Une espace insécable avant ;
        article.replace(" :","&nbsp;:"); // Une espace insécable avant :
        article.replace(" ?","&nbsp;?"); // Une espace insécable avant ?
        article.replace(" !","&nbsp;!"); // Une espace insécable avant !
        article.replace(" »","&nbsp;»"); // Une espace insécable avant »
        article.replace("« ","«&nbsp;"); // Une espace insécable après «
//        qDebug() << article;
//        int p = article.indexOf("<i><b>Moy.</b></i>");
//        if (article.mid(p-1,1) != "\n") article.insert(p,"<br/>\n");
        article.replace("\t"," "); // le tab
        if (article.contains("v.") || article.contains("c.") || article.contains("p."))
        {
            // Il y a probablement des renvois
//            qDebug() << article;
            article = lierRenvois(article, " v.");
            article = lierRenvois(article, " c.");
            article = lierRenvois(article, " p.");
            article = lierRenvois(article, ">p.</i> ");
            article = lierRenvois(article, ">v.</i> ");
            article = lierRenvois(article, ">c.</i> ");
            article = lierRenvois(article, ">p.</i>&nbsp;");
            article = lierRenvois(article, ">v.</i>&nbsp;");
            article = lierRenvois(article, ">c.</i>&nbsp;");
//            qDebug() << article;
        }
        res[i] = article;
//        qDebug() << article;
    }
    return res;
}

/**
 * @brief consultation du Pape
 * @param f : la forme à chercher
 * @return une liste avec les articles correspondants (en HTML)
 *
 * Cette fonction appelle la fonction générique Lemmat::consult
 * et essaie ensuite d'ajouter des liens à chaque fois qu'elle
 * pense avoir trouvé un renvoi (dont la forme dépend du dictionnaire).
 */
QStringList Lemmat::consPape(QString f)
{
    // Consulter le Pape pour la forme f

    QStringList llem = cherchIndex(f, &_PapeIndex);

    if (llem.isEmpty()) return llem;
    QStringList res = consult(_PapeName,llem,"P");
    // Je fais du post-traitement : recherche des renvois.
    if (!res.isEmpty()) for (int i=3; i<res.size();i++)
    {
        // Les 3 premiers éléments donnent le mot avant, le mot après et les liens.
        QString article = res[i];
        article.replace("\t"," "); // le tab
        article.replace("=\u00a0","= "); // l'espace insécable
        if (article.contains("s.") || article.contains("=") || article.contains("vlg.") || article.contains(". für "))
        {
            // Il y a probablement des renvois
            article = lierRenvois(article, " = ");
//            article = lierRenvois(article, " <i>= ");
            article = lierRenvois(article, " s. ");
            article = lierRenvois(article, "(s. ");
            article = lierRenvois(article, ". für ");
            article = lierRenvois(article, "(vlg. ");
            res[i] = article;
        }
    }
    return res;
}

/**
 * @brief consultation d'un dictionnaire
 * @param nom : le nom du fichier pour le dictionnaire
 * @param llem : la liste des entrées de l'index que l'on souhaite afficher
 * @param prefix : un préfixe pour s'y retrouver dans les liens sur la page finale.
 * @return une liste de chaines de caractères
 *
 * La consultation d'un dictionnaire se prépare en trois temps.
 * D'abord, on cherche une forme dans l'index du dictionnaire.
 * Ensuite, on prend la liste des entrées correspondantes et
 * on va chercher, dans le fichier contenant le dictionnaire,
 * les articles correspondants.
 * Enfin, on essaie d'identifier des renvois dans chaque article.
 *
 * La liste retournée a un format un peu particulier.
 * Si elle n'est pas vide (la forme a été trouvée dans le dictionnaire),
 * elle contient au moins quatre éléments.
 * Les trois premiers sont respectivement le mot avant, le mot après
 * et la liste de liens. Ils permettront donc la navigation entre
 * les articles. Ensuite viennent les articles de dictionnaires.
 */
QStringList Lemmat::consult(QString nom, QStringList llem, QString prefix)
{
    QStringList ici;
    QStringList donnees;
    QStringList avant;
    QStringList apres;
    QStringList liens;
    QStringList positions;
    QString div = "<div id='#" + prefix + "%1'>";
    QString aHref = "<a href='§" + prefix + "%1'>%2</a> ";
    // Normalement, c'est un # pour aller à une ancre.
    // Mais il y a un conflit avec les "#1" etc... utilisés en betacode.
    if (!QFile::exists(_rscrDir + nom)) return liens;
    QFile fichier (_rscrDir + nom);
    fichier.open(QFile::ReadOnly | QFile::Text);
//qDebug() << llem.size();
    for (int i = llem.size() - 1; i > -1; i--)
    {
        QStringList ecl = llem[i].split(":");
//        qDebug() << ecl.size() << llem[i];
        if (!positions.contains(ecl[1]))
        {
            // Le Bailly peut avoir plusieurs lemmes qui pointent
            // sur un même article. Avec l'*, on peut avoir collecté ces lemmes :
            // il ne faut pas afficher deux fois le même article.
            positions.append(ecl[1]);
            qint64 p = ecl[1].toInt();
            fichier.seek(p);
            QString linea = fichier.readLine();
            //        qDebug() << llem[i] << linea;
            ici.append(ecl[2]);
            avant.append(ecl[3]);
            apres.append(ecl[4]);
            liens.append(aHref.arg(i).arg(ecl[2]));

            if (linea.startsWith("<div"))
            {
                // C'est le Bailly 2020
                linea = linea.section(">",1); // Je supprime le "<div...>"
                linea.replace("'","’"); // pour avoir de jolies apostrophes...
                //            linea.replace("’color:red’","'color:red'");
            }
            else
            {
                linea.insert(ecl[2].size(),"</b></span>");
                linea.prepend("<span style='color:red'><b>");
                linea.append("</div>");
            }
            linea.prepend(div.arg(i));
            donnees.append(linea);
        }
    }
    fichier.close();
//    qDebug() << avant << apres;
    for (int i = ici.size() - 1; i > -1; i--)
    {
        if (ici.contains(avant[i])) avant.removeAt(i);
        if (ici.contains(apres[i])) apres.removeAt(i);
        // J'élimine les mots qui sont déjà affichés.
    }
//    qDebug() << avant << apres;
    QString prems = avant[0];
    if (avant.size() > 1) for (int i=1; i<avant.size();i++)
        if (prems < avant[i]) prems = avant[i];
    QString der = apres[0];
    if (apres.size() > 1) for (int i=1; i<apres.size();i++)
        if (der > apres[i]) der = apres[i];
    QString lgLiens = "<big>" + liens.join(" ") + "</big>";
    lgLiens.prepend(" <a href='http://aller/" + uni2betacode(prems) + "'>" + prems + "</a> ");
    lgLiens.append(" <a href='http://aller/" + uni2betacode(der) + "'>" + der + "</a> ");
//    qDebug() << lgLiens;
    donnees.prepend(lgLiens);
    donnees.prepend(der);
    donnees.prepend(prems);
    return donnees;
}

/**
 * @brief mise à jour du Pape
 * @param nom : le chemin complet du nouveau fichier
 *
 * Dans Eulexis, les dictionnaires sont simplement en HTML.
 * N'importe qui peut donc les corriger à chaque fois qu'une erreur est trouvée.
 * Quand on est satisfait des corrections apportées sur une **copie de travail**,
 * on peut l'importer dans Eulexis. Cette routine fait suite à
 * MainWindow::majP qui a recopié la copie de travail au bon endroit et
 * elle va lire le dictionnaire et reconstruire l'index correspondant.
 *
 * @attention Ne jamais travailler directement sur le fichier utilisé par Eulexis !
 * @attention Toujours conserver une copie du fichier d'origine
 * (on sait qu'il est conforme aux attentes du programme).
 */
void Lemmat::majPape(QString nom)
{
    // Pour le dico "Pape*.txt"
    QFile fandr (_rscrDir + nom);
    QFile findex (_rscrDir + "Pape.csv");
    QString cle_prec="ὠώδης"; // dernier mot du Pape
    if (fandr.open (QFile::ReadOnly | QFile::Text))
    {
        QString linea;
        QString cle;
        findex.open(QFile::WriteOnly | QFile::Text);
        QTextStream fli (&findex);
        fli.setCodec ("UTF-8");
        fli << "! " << fandr.fileName().section("/",-1) << "\n!\n";
        fli << QString("! Fichier d'index pour le dico sus-nommé\n");
        fli << QString("! créé par et pour la version résidente d'Eulexis.\n!\n");
        fli << QString("! Le fichier HTML du Pape a été revu Chaerephon.\n!\n");
        fli << "! Philippe Verkerk 2020.\n!\n";
        fli << QString("! Les lignes commençant par un ! sont des commentaires,\n!");
        qint64 p;
        fandr.seek (0);
        //    QStringList tous;
        while (!fandr.atEnd ())
        {
            // flux.flush ();
            p = fandr.pos ();
            linea = fandr.readLine ();
            if (linea.startsWith('!') || linea.isEmpty()) continue;
            linea=linea.trimmed();
            // Pour le Pape, le premier mot avant un double blanc
//            if (linea.count("<i>") != linea.count("</i>")) qDebug() << linea;
            verif(linea);
            cle = linea.section("\t",0,0).trimmed();
            if (cle.contains(reLettres)) qDebug() << cle << linea;
            if (cle.size() > 20) cle = cle.mid(0,20);
            cle.remove(",");
            QString clef = nettoie(cle).toLower();
            // La clef est la version, en caractères latins sans décoration, de cle (en grec avec déco).
            if (clef.contains("-"))
            {
                if (clef.startsWith("-")) clef = clef.mid(1).trimmed();
                else clef.remove("-");
//                else clef = clef.section("-",0,0).trimmed();
            }
            if (clef.contains("–"))
            {
                if (clef.startsWith("–")) clef = clef.mid(1).trimmed();
                else clef = clef.section("–",0,0).trimmed();
            }
            if (clef.contains(" ")) clef = clef.section(" ",0,0);
            if (clef.contains(",")) clef = clef.section(",",0,0);
            clef.remove("¹");
            clef.remove("²");
            clef.remove("³");
            clef.remove("⁴");
            clef.remove("⁵");

            fli << cle << "\n" << clef << ":" << p << ":" << cle << ":" << cle_prec << ":";
            // La ligne est incomplète : la clé suivante viendra la compléter.
            cle_prec = cle;
        }
        fli << QString("Α\n"); // Compléter la dernière ligne avec le premier mot.
    }
    else qDebug() << "erreur";
    fandr.close ();
    findex.close ();
    lirePape();
}

/**
 * @brief lie les renvois à d'autres mots
 * @param article : un article du dictionnaire
 * @param renvoi : une forme qui introduit parfois un renvoi, par exemple " v. "
 * @return l'article d'origine dans lequel on a inséré des liens sur les renvois.
 */
QString Lemmat::lierRenvois(QString article, QString renvoi)
{
    int p = article.indexOf(renvoi);
    int l = renvoi.size();
    while (p > 0)
    {
        // p pointe sur l'espace qui précède un v.
        QString mot = article.mid(p + l).trimmed();
        while (!mot.isEmpty() && mot.startsWith("<"))
        {
            mot = mot.mid(mot.indexOf(">") + 1).trimmed();
        }
        if (mot.contains(" ")) mot = mot.section(" ",0,0);
        if (mot.contains("\n")) mot = mot.section("\n",0,0);
        if (mot.contains("<")) mot = mot.section("<",0,0);
        mot.remove(rePonct);
        if (!mot.isEmpty() && !mot.contains(reLettres) && !mot[0].isDigit())
        {
            // C'est un mot grec !
            // Est-il dans le dico ?
            QStringList llem = _LSJindex.values(nettoie(mot));
            if (llem.isEmpty()) llem = _PapeIndex.values(nettoie(mot));
            if (llem.isEmpty()) llem = _BaillyIndex.values(nettoie(mot));
            if (!llem.isEmpty())
            {
                QString ajout = "<a href='http://aller/" + uni2betacode(mot) + "'>";
                int pp = article.indexOf(mot, p); // La position du mot grec qui suit le renvoi.
                if (pp > 0)
                {
                    article.insert(pp + mot.size(),"</a>");
                    article.insert(pp,ajout);
                    p = pp + 4 + ajout.size() + mot.size();
                }
            }
        }
        p = article.indexOf(renvoi,p+1);
//        qDebug() << mot << p << article.size();
    }
    return article;
}

/**
 * @brief essaie de réconcilier les préfixes et le lemme
 * @param beta : le lemme et ses préfixes en betacode
 * @return une forme plausible du lemme complet
 *
 * Perseus a séparé les préfixes et ne considère que la traduction
 * du lemme racine. Dans la vérification des traductions,
 * j'essaie de trouver les formes composées dans les dicos.
 * Mais les préverbes changent de forme.
 * Par exemple, su/n-p... devient sump...
 * @note probablement incomplet et inutile.
 */
QString Lemmat::reconcil(QString beta)
{
    QString pre = beta.section("-",0,0);
    QString verb = beta.section("-",1);
    if (verb.contains("-")) verb = reconcil(verb);
    if (pre.endsWith("/")) pre.chop(1);
    if (voyelles.contains(pre[pre.size() - 1]) && voyelles.contains(verb[0])
            && !pre.endsWith("peri")) // peri subsiste
    {
        pre.chop(1);
        if ((verb[1] == '(') || (voyelles.contains(verb[1]) && (verb[2] == '(')))
        {
            // Changement de consonne avant un esprit rude
            if (pre[pre.size() - 1] == 'p') pre[pre.size() - 1] = 'f'; // pi devient phi
            if (pre[pre.size() - 1] == 't') pre[pre.size() - 1] = 'q'; // tau devient tetha
            if (pre[pre.size() - 1] == 'k') pre[pre.size() - 1] = 'c'; // kappa devient xhi
        }
    }
    else
    {
        // quelques changements avec su/n
        if (pre.endsWith("n"))
        {
            if (verb.startsWith("b") || verb.startsWith("p") || verb.startsWith("m")
                                  || verb.startsWith("f") || verb.startsWith("y"))
                pre[pre.size() - 1] = 'm';
            if (verb.startsWith("l"))
                pre[pre.size() - 1] = 'l';
            if (verb.startsWith("r"))
                pre[pre.size() - 1] = 'r';
        }
    }
    return pre + verb;
}

/**
 * @brief construit l'index commun aux quatre dictionnaires pour la version web d'Eulexis
 *
 * Dans la version web, j'utilise un index unique qui regroupe les index
 * des quatre dictionnaires, séparés ici.
 * Pour que la version web bénéficie des mise à jour sur les dictionnaires,
 * je construis ici cet index commun que j'exporte ensuite avec
 * les autres fichiers concernés.
 * @note Je suis (et serai probablement toujours) le seul à utiliser cette fonction.
 */
void Lemmat::indexCommun()
#ifdef PIRATE
{
    // Je pirate la construction de l'index commun pour faire un outil d'alignement
    if (_toInit) initData();
    QString racine = "/Users/Philippe/Documents/GIT/Alpheios_Bailly/data/raw/eulexis/";
    QMultiMap<QString,QString> sens;
    QMultiMap<QString,QString> subst;
    QMultiMap<QString,QString> renv;
    QMultiMap<QString,QString> es_sens;
    QMultiMap<QString,QString> es_renv;
    QMultiMap<QString,QString> sens_sd;
    QMultiMap<QString,QString> subst_sd;
    QMultiMap<QString,QString> renv_sd;
    QMultiMap<QString,QString> es_sens_sd;
    QMultiMap<QString,QString> es_renv_sd;
    QMultiMap<QString,QString> nbOcc;
    QMultiMap<QString,QString> nbOcc_sd;
    QString lg;
    QString lem;
    QString beta;
    // J'ajoute un morceau pour faire la liste des pseudo-homonymes.
    QMultiMap <QString,QString> pseudos;
    QStringList complexes;
    foreach (QString lem, _trad.keys())
    {
        QString beta = nettoie2(lem);
        if (beta[beta.size() - 1].isDigit()) beta.chop(1);
        pseudos.insert(beta,lem);
        // Je regroupe les lemmes en fonction des chaines sans diacritique.
    }
    QStringList ps = pseudos.keys();
    ps.removeDuplicates();
/*    QFile fListe("/Users/Philippe/Documents/Chantiers_Eulexis/pseudos.csv");
    if (!fListe.open(QFile::WriteOnly | QFile::Text))
        qDebug() << "Echec";
    QTextStream fluxL(&fListe);
    fluxL.setCodec("UTF-8");*/
    foreach (QString lem, ps) {
        QStringList liste = pseudos.values(lem);
        if (liste.size() > 1)
            complexes.append(liste);
//            fluxL << liste.size() << "\t" << liste.join("\t") << "\n";
    }
    qDebug() << complexes.size();
//    fListe.close();
    // Fin de l'ajout (5 février 2021)
    QFile fichier (racine + "Bailly_sens.csv");
    fichier.open(QFile::ReadOnly | QFile::Text);
    while (!fichier.atEnd())
    {
        lg = fichier.readLine();
        if (lg.endsWith("\n")) lg.chop(1);
        if (lg.isEmpty()) continue;
        lem = lg.section("\t",0,0);
        beta = lem;
        if (beta[0].isDigit()) beta = beta.mid(2);
        if (beta[0] == '*') beta = beta.mid(1);
        if (beta.contains(" ")) beta = beta.section(" ",0,0);
        if (beta.endsWith(",")) beta.chop(1);
        sens.insert(uni2betacode(beta),lg);
        sens_sd.insert(nettoie(beta),lg);
    }
    fichier.close();
    qDebug() << "Sens chargés";
    fichier.setFileName(racine + "Bailly_renv.csv");
    fichier.open(QFile::ReadOnly | QFile::Text);
    while (!fichier.atEnd())
    {
        lg = fichier.readLine();
        if (lg.endsWith("\n")) lg.chop(1);
        if (lg.isEmpty()) continue;
        lem = lg.section("\t",0,0);
        beta = lem;
        if (beta[0].isDigit()) beta = beta.mid(2);
        if (beta[0] == '*') beta = beta.mid(1);
        if (beta.contains(" ")) beta = beta.section(" ",0,0);
        renv.insert(uni2betacode(beta),lg);
        renv_sd.insert(nettoie(beta),lg);
        // Je ne cherche pas à résoudre les renvois
    }
    fichier.close();
    qDebug() << "Renv chargés";
    fichier.setFileName(racine + "Logeion_freq_sup5.csv");
    fichier.open(QFile::ReadOnly | QFile::Text);
    while (!fichier.atEnd())
    {
        lg = fichier.readLine();
        if (lg.endsWith("\n")) lg.chop(1);
        if (lg.isEmpty()) continue;
        lem = lg.section("\t",0,0);
        beta = lem;
        if (beta[beta.size() - 1].isDigit()) beta.chop(1);
        if (beta[0] == '*') beta = beta.mid(1);
        if (beta.contains(" ")) beta = beta.section(" ",0,0);
        nbOcc.insert(uni2betacode(beta),lg);
        nbOcc_sd.insert(nettoie(beta),lg);
    }
    fichier.close();
    qDebug() << "Nombres d'occurrences chargés";
    fichier.setFileName(racine + "Bailly_subst.csv");
    fichier.open(QFile::ReadOnly | QFile::Text);
    while (!fichier.atEnd())
    {
        lg = fichier.readLine();
        if (lg.endsWith("\n")) lg.chop(1);
        if (lg.isEmpty()) continue;
        lem = lg.section("\t",1,1);
        beta = lem;
        if (beta[0].isDigit()) beta = beta.mid(2);
        if (beta[0] == '*') beta = beta.mid(1);
        if (beta.contains(" ")) beta = beta.section(" ",1,1);
        // Je dois recomposer la ligne autrement (comme pour Bailly_sens)
        QStringList eclats = lg.split("\t");
        QString toto = "\t%1\t";
        lg = eclats[0] + " " + eclats[1] + " (in " + eclats[3] + ")\t";
        lg = lg + eclats[2] + toto.arg(eclats[2].size()) + eclats[4];
        subst.insert(uni2betacode(beta),lg);
        subst_sd.insert(nettoie(beta),lg);
    }
    fichier.close();
    qDebug() << "Subst chargés";
    fichier.setFileName(racine + "Bailly_es_renv.csv");
    fichier.open(QFile::ReadOnly | QFile::Text);
    while (!fichier.atEnd())
    {
        lg = fichier.readLine();
        if (lg.endsWith("\n")) lg.chop(1);
        if (lg.isEmpty()) continue;
        lem = lg.section("\t",1,1);
        if (lem.contains(","))
        {
            // Il y en a plusieurs
            QStringList e = lem.split(", ");
            foreach (beta, e)
            {
                if (beta[0].isDigit()) beta = beta.mid(2);
                if (beta[0] == '*') beta = beta.mid(1);
                if (beta.contains(" ")) beta = beta.section(" ",1,1);
                es_renv.insert(uni2betacode(beta),lg);
                es_renv_sd.insert(nettoie(beta),lg);
            }
        }
        else
        {
            beta = lem;
            if (beta[0].isDigit()) beta = beta.mid(2);
            if (beta[0] == '*') beta = beta.mid(1);
            if (beta.contains(" ")) beta = beta.section(" ",0,0);
            es_renv.insert(uni2betacode(beta),lg);
            es_renv_sd.insert(nettoie(beta),lg);
        }
    }
    fichier.close();
    qDebug() << "Es Renv chargés";
    fichier.setFileName(racine + "Bailly_es_sens.csv");
    fichier.open(QFile::ReadOnly | QFile::Text);
    while (!fichier.atEnd())
    {
        lg = fichier.readLine();
        if (lg.endsWith("\n")) lg.chop(1);
        if (lg.isEmpty()) continue;
        lem = lg.section("\t",1,1);
        if (lem.contains(","))
        {
            // Il y en a plusieurs
            QStringList e = lem.split(", ");
            foreach (beta, e)
            {
                if (beta[0].isDigit()) beta = beta.mid(2);
                if (beta[0] == '*') beta = beta.mid(1);
                if (beta.contains(" ")) beta = beta.section(" ",1,1);
                es_sens.insert(uni2betacode(beta),lg);
                es_sens_sd.insert(nettoie(beta),lg);
            }
        }
        else
        {
            beta = lem;
            if (beta[0].isDigit()) beta = beta.mid(2);
            if (beta[0] == '*') beta = beta.mid(1);
            if (beta.contains(" ")) beta = beta.section(" ",1,1);
            es_sens.insert(uni2betacode(beta),lg);
            es_sens_sd.insert(nettoie(beta),lg);
        }
    }
    fichier.close();
    qDebug() << "es Sens chargés";
    int nbre;
    int numero = 0;
    int numbis = 0;
    QStringList lesRes;
    QString format = "%1\t%2\t%3\t%4\t";
    fichier.setFileName("/Users/Philippe/Documents/GIT/Alpheios_Bailly/data/Eulexis_Bailly_sens.csv");
    fichier.open(QFile::WriteOnly | QFile::Text);
    QTextStream flux (&fichier);
    flux.setCodec("UTF-8");
    flux << "numero\tlemme\tbetacode\ttrad_En\tlem_tr\tsens\tlongueur\tindications\toccurrences\tnombre\n";
    QFile fout("/Users/Philippe/Documents/Chantiers_Eulexis/to_check_first.csv");
    fout.open(QFile::WriteOnly | QFile::Text);
    QTextStream sout (&fout);
    sout.setCodec("UTF-8");
    QFile fout2("/Users/Philippe/Documents/Chantiers_Eulexis/to_check_later.csv");
    fout2.open(QFile::WriteOnly | QFile::Text);
    QTextStream sout2 (&fout2);
    sout2.setCodec("UTF-8");
    QFile fout3("/Users/Philippe/Documents/Chantiers_Eulexis/complexCases.csv");
    fout3.open(QFile::WriteOnly | QFile::Text);
    QTextStream sout3 (&fout3);
    sout3.setCodec("UTF-8");

    foreach (beta, _trad.keys())
    {
        lesRes.clear();
        numero++;
        // Je prends chaque lemme de mon lexique
        lem = beta2unicode(beta, false);
        QString deb = format.arg(numero).arg(lem).arg(beta).arg(_trad[beta].section("\t",0,0));
        QString fin = "\t";
        QString fin2 = "\t" + _trad[beta].section("\t",1,2) + "\t";
        // Je me prépare pour générer un fichier de travail avec les traductions existantes.
        bool freq = false;
        // le début et la fin de ma ligne.
        if (beta[beta.size()-1].isDigit()) beta.chop(1);
        else numbis++;
        // numbis me servira pour répartir les lemmes
        // et je voudrais qu'une même personne traite tous les homonymes.
        if (beta.contains("-")) beta = reconcil(beta);
        // Il faudrait réconcilier le radical et le préverbe.
        nbre = 0;
        if (nbOcc.contains(beta))
        {
            // Ce lemme a une chance d'avoir une fréquence ≥ 5
            freq = true;
            QStringList ls = nbOcc.values(beta);
            if (ls.size() == 1 || !lem[lem.size()-1].isDigit())
            {
                fin.append(ls[0].section("\t",1,1));
                fin2.append(ls[0].section("\t",1,1));
            }
            // C'est le cas simple où je n'ai qu'une solution ou
            // que mon lemme n'a pas de numéro d'homonymie
            else
            {
                // S'il y a plusieurs solutions, je regarde si je peux démêler les homonymes
                int ii = 0;
                while ((ii < ls.size()) && (lem[lem.size()-1] != ls[ii][ls[ii].size()-1]))
                    ii++;
                if (ii < ls.size())
                {
                    fin.append(ls[ii].section("\t",1,1));
                    fin2.append(ls[ii].section("\t",1,1));
                }
                else
                {
                    fin.append(ls[0].section("\t",1,1));
                    fin2.append(ls[0].section("\t",1,1));
                }
            }
        }
        else if (nbOcc_sd.contains(beta))
        {
            freq = true;
            // Ce lemme a une chance d'avoir une fréquence ≥ 5
            QStringList ls = nbOcc_sd.values(beta);
            fin.append("#" + ls[0].section("\t",1,1));
            fin2.append("#" + ls[0].section("\t",1,1));
            // Je n'ai pas de solution exacte ! Mais je valide quand même.
        }
        else
        {
            fin.append("3"); // J'attribue une fréquence de 3 à tous les autres lemmes
            fin2.append("3");
        }
        fin.append("\t");
        fin2.append("\t");

        if (sens.contains(beta))
        {
            // Combien ?
            QStringList ls = sens.values(beta);
            nbre += ls.size();
            lesRes.append(ls);
        }
        if (renv.contains(beta))
        {
            // Combien ?
            QStringList ls = renv.values(beta);
            nbre += ls.size();
            // Je dois suivre le renvoi et donner le sens trouvé.
            for (int i = 0; i < ls.size(); i++)
            {
                QStringList eclats = ls[i].split("\t");
                QString lr = uni2betacode(eclats[1].mid(2));
                bool encore = true;
                int iter = 0;
                while (encore && (iter < 10))
                { // Boucle pour les renvois à des renvois
                    if (lr.contains(", ")) lr = lr.section(", ",0,0);
                    if (lr.endsWith(".")) lr.chop(1);
                    if (lr[0].isDigit()) lr = lr.mid(2);
                    if (lr[lr.size() - 1].isDigit()) lr.chop(2);
                    // Les renvois vers des homonymes sont de la forme > lemme 1
                    if (eclats[1].startsWith("> *")) lr = lr.mid(1);
                    if (lr.contains(" ")) lr = lr.section(" ",0,0);
                    if (lr.endsWith(",")) lr.chop(1);
                    if (sens.contains(lr))
                    {
                        encore = false;
                        QStringList lss = sens.values(lr);
                        for (int ii = 0; ii < lss.size(); ii++)
                            if (!eclats[1][eclats[1].size()-1].isDigit() ||
                                    (lss[ii].at(0) == eclats[1][eclats[1].size()-1])
                                    || (lss.size() == 1))
                            {
                                lg = lss[ii].section("\t",0,2);
                                lg.prepend(eclats[0] + " > ");
                                lg.append("\t" + eclats[2]);
                                lesRes.append(lg);
                            }
                    }
                    else if (subst.contains(lr))
                    {
                        encore = false;
                        QStringList lss = subst.values(lr);
                        for (int ii = 0; ii < lss.size(); ii++)
                        {
                            lg = lss[ii].section("\t",0,2);
                            lg.prepend(eclats[0] + " > ");
                            lg.append("\t" + eclats[2]);
                            lesRes.append(lg);
                        }
                    }
                    else if (renv.contains(lr))
                    {
                        iter++;
                        QStringList lss = renv.values(lr);
                        if ((lss.size() == 1) && (iter < 10))
                            lr = uni2betacode(lss[0].section("\t",1,1).mid(2));
                        else
                        {
                            encore = false;
                            lg = eclats[0] + " " + eclats[1] + "\t???? Renvoi sans Issue 1 ????\t0\t" + eclats[2];
                            lesRes.append(lg);
                            qDebug() << lg << lr << iter << lss;
                        }
                    }
                    else
                    {
                        encore = false;
                        lg = eclats[0] + " " + eclats[1] + "\t???? Renvoi sans Issue 2 ????\t0\t" + eclats[2];
                        lesRes.append(lg);
//                        qDebug() << lg << lr << iter << ls;
                    }
                } // Fin de la boucle pour les renvois à des renvois.
            } // Fin de l'exploration des tous les renvois.
        }
        if (subst.contains(beta))
        {
            // Combien ?
            QStringList ls = subst.values(beta);
            nbre += ls.size();
            lesRes.append(ls);
        }
        if (es_sens.contains(beta))
        {
            // Combien ?
            QStringList ls = es_sens.values(beta);
            nbre += ls.size();
            for (int i = 0; i < ls.size(); i++)
            {
                QStringList eclats = ls[i].split("\t");
                QString lr = uni2betacode(eclats[0]);
                if (lr[lr.size()-1].isDigit()) lr.chop(1);
                // J'avais mis le numéro à la fin
                if (eclats[0].startsWith('*')) lr = lr.mid(1);
                if (lr.contains(" ")) lr = lr.section(" ",0,0);
                if (sens.contains(lr))
                {
                    QStringList lss = sens.values(lr);
                    for (int ii = 0; ii < lss.size(); ii++)
                        if (!eclats[0][eclats[0].size()-1].isDigit() ||
                                (lss[ii].at(0) == eclats[0][eclats[0].size()-1]))
                    {
                        lg = lss[ii];
                        lg.prepend(eclats[1] + " < ");
                        // L'entrée secondaire était dans l'article.
                        lesRes.append(lg);
                    }
                }
                else
                {
                    lg = eclats[0] + " avec " + eclats[1] + "\t???? Article Perdu ????\t0\t";
                    lesRes.append(lg);
                }
            }
        }
        if (es_renv.contains(beta))
        {
            // Combien ?
            QStringList ls = es_renv.values(beta);
            nbre += ls.size();
            for (int i = 0; i < ls.size(); i++)
            {
                QStringList eclats = ls[i].split("\t");
                QString lr = uni2betacode(eclats[3]);
                if (eclats[3].startsWith('*')) lr = lr.mid(1);
                if (lr.contains(", ") && (lr.count(", ") == eclats[1].count(", ") + 1))
                {
                    // il y a plusieurs renvois, lequel est le bon...
//                    qDebug() << beta << lr << eclats[1];
                    QStringList e = eclats[1].split(", ");
                    int jj = 0;
                    while ((jj < e.size()) && (beta != uni2betacode(e[jj]))) jj++;
                    if (beta == uni2betacode(e[jj])) lr = lr.section(", ",jj + 1, jj + 1);
//                    qDebug() << beta << lr << eclats[1] << jj << e.size();
                }
                // C'est une es d'un article avec renvoi :
                // je vais directement au renvoi.
                if (lr[lr.size()-1].isDigit()) lr.chop(1);
                // J'avais mis le numéro à la fin
                if (lr.contains(" ")) lr = lr.section(" ",0,0);
                if (sens.contains(lr))
                {
                    QStringList lss = sens.values(lr);
                    for (int ii = 0; ii < lss.size(); ii++)
                        if (!eclats[3][eclats[3].size()-1].isDigit() ||
                                (lss[ii].at(0) == eclats[3][eclats[3].size()-1]))
                        {
                            lg = lss[ii];
                            lg.prepend(eclats[1] + " < " + eclats[0] + " > ");
                            // L'entrée secondaire était dans l'article qui a renvoyé.
                            lesRes.append(lg);
                        }
                }
                else
                {
                    lg = eclats[0] + " avec " + eclats[1] + " > " + eclats[3] + "\t???? Article Perdu ????\t0\t";
                    lesRes.append(lg);
                }

            }
        }
        if (nbre == 0)
        {
            nbre = 100;
            beta = nettoie2(beta);
            if (sens_sd.contains(beta))
            {
                // Combien ?
                QStringList ls = sens_sd.values(beta);
                nbre += ls.size();
                lesRes.append(ls);
            }
            if (renv_sd.contains(beta))
            {
                // Combien ?
                QStringList ls = renv_sd.values(beta);
                nbre += ls.size();
                // Je dois suivre le renvoi et donner le sens trouvé.
                for (int i = 0; i < ls.size(); i++)
                {
                    QStringList eclats = ls[i].split("\t");
                    QString lr = uni2betacode(eclats[1].mid(2));
                    bool encore = true;
                    int iter = 0;
                    while (encore && (iter < 10))
                    { // Boucle pour les renvois à des renvois
                        if (lr.contains(", ")) lr = lr.section(", ",0,0);
                        if (lr.endsWith(".")) lr.chop(1);
                        if (lr[0].isDigit()) lr = lr.mid(2);
                        if (lr[lr.size() - 1].isDigit()) lr.chop(2);
                        // Les renvois vers des homonymes sont de la forme > lemme 1
                        // Mais parfois le 1 est là pour dire "le point 1 de l'article)
                        if (eclats[1].startsWith("> *")) lr = lr.mid(1);
                        if (lr.contains(" ")) lr = lr.section(" ",0,0);
                        if (lr.endsWith(",")) lr.chop(1);
                        if (sens.contains(lr))
                        {
                            encore = false;
                            QStringList lss = sens.values(lr);
                            for (int ii = 0; ii < lss.size(); ii++)
                                if (!eclats[1][eclats[1].size()-1].isDigit() ||
                                        (lss[ii].at(0) == eclats[1][eclats[1].size()-1])
                                        || (lss.size() == 1))
                                {
                                    lg = lss[ii].section("\t",0,2);
                                    lg.prepend(eclats[0] + " > ");
                                    lg.append("\t" + eclats[2]);
                                    lesRes.append(lg);
                                }
                        }
                        else if (subst.contains(lr))
                        {
                            encore = false;
                            QStringList lss = subst.values(lr);
                            for (int ii = 0; ii < lss.size(); ii++)
                            {
                                lg = lss[ii].section("\t",0,2);
                                lg.prepend(eclats[0] + " > ");
                                lg.append("\t" + eclats[2]);
                                lesRes.append(lg);
                            }
                        }
                        else if (renv.contains(lr))
                        {
                            iter++;
                            QStringList lss = renv.values(lr);
                            if ((lss.size() == 1) && (iter < 10))
                                lr = uni2betacode(lss[0].section("\t",1,1).mid(2));
                            else
                            {
                                encore = false;
                                lg = eclats[0] + " " + eclats[1] + "\t???? Renvoi sans Issue 3 ????\t0\t" + eclats[2];
                                lesRes.append(lg);
                                qDebug() << lg << lr << iter << lss;
                            }
                        }
                        else
                        {
                            encore = false;
                            lg = eclats[0] + " " + eclats[1] + "\t???? Renvoi sans Issue 4 ????\t0\t" + eclats[2];
                            lesRes.append(lg);
//                            qDebug() << lg << lr << iter << ls;
                        }
                    } // Fin de la boucle pour les renvois à des renvois.
                } // Fin de l'exploration des tous les renvois.
            }
            if (subst_sd.contains(beta))
            {
                // Combien ?
                QStringList ls = subst_sd.values(beta);
                nbre += ls.size();
                lesRes.append(ls);
            }
            if (es_sens_sd.contains(beta))
            {
                // Combien ?
                QStringList ls = es_sens_sd.values(beta);
                nbre += ls.size();
                for (int i = 0; i < ls.size(); i++)
                {
                    QStringList eclats = ls[i].split("\t");
                    QString lr = uni2betacode(eclats[0]);
                    if (lr[lr.size()-1].isDigit()) lr.chop(1);
                    // J'avais mis le numéro à la fin
                    if (eclats[0].startsWith('*')) lr = lr.mid(1);
                    if (lr.contains(" ")) lr = lr.section(" ",0,0);
                    if (sens.contains(lr))
                    {
                        QStringList lss = sens.values(lr);
                        for (int ii = 0; ii < lss.size(); ii++)
                            if (!eclats[0][eclats[0].size()-1].isDigit() ||
                                    (lss[ii].at(0) == eclats[0][eclats[0].size()-1]))
                        {
                            lg = lss[ii];
                            lg.prepend(eclats[1] + " < ");
                            // L'entrée secondaire était dans l'article.
                            lesRes.append(lg);
                        }
                    }
                    else
                    {
                        lg = eclats[0] + " avec " + eclats[1] + "\t???? Article Perdu ????\t0\t";
                        lesRes.append(lg);
                    }
                }
            }
            if (es_renv_sd.contains(beta))
            {
                // Combien ?
                QStringList ls = es_renv_sd.values(beta);
                nbre += ls.size();
                for (int i = 0; i < ls.size(); i++)
                {
                    QStringList eclats = ls[i].split("\t");
                    QString lr = uni2betacode(eclats[3]);
                    if (eclats[3].startsWith('*')) lr = lr.mid(1);
                    if (lr.contains(", ") && (lr.count(", ") == eclats[1].count(", ") + 1))
                    {
                        // il y a plusieurs renvois, lequel est le bon...
                        QStringList e = eclats[1].split(", ");
                        int jj = 0;
                        while ((jj < e.size()) && (beta != nettoie(e[jj]))) jj++;
                        if (beta == nettoie(e[jj])) lr = lr.section(", ",jj + 1, jj + 1);
                    }
                    // C'est une es d'un article avec renvoi :
                    // je vais directement au renvoi.
                    if (lr[lr.size()-1].isDigit()) lr.chop(1);
                    // J'avais mis le numéro à la fin
                    if (lr.contains(" ")) lr = lr.section(" ",0,0);
                    if (sens.contains(lr))
                    {
                        QStringList lss = sens.values(lr);
                        for (int ii = 0; ii < lss.size(); ii++)
                            if (!eclats[3][eclats[3].size()-1].isDigit() ||
                                    (lss[ii].at(0) == eclats[3][eclats[3].size()-1]))
                        {
                            lg = lss[ii];
                            lg.prepend(eclats[1] + " < " + eclats[0] + " > ");
                            // L'entrée secondaire était dans l'article qui a renvoyé.
                            lesRes.append(lg);
                        }
                    }
                    else
                    {
                        lg = eclats[0] + " avec " + eclats[1] + " > " + eclats[3] + "\t???? Article Perdu ????\t0\t";
                        lesRes.append(lg);
                    }
                }
            }
        } // Fin de la deuxième chance.
        if (lesRes.isEmpty())
        {
            flux << "! " << deb << "\t\t\t" << fin << nbre << "\n";
            if (complexes.contains(uni2betacode(lem)))
                sout3 << "! " << deb << "\t" << fin2 << (numbis & 1023) << "\t" << nettoie(lem) << "\n";
            else if (freq && !deb.contains("-"))
                sout << "! " << deb << "\t" << fin2 << (numbis & 1023) << "\n";
            else sout2 << "! " << deb << "\t" << fin2 << (numbis & 1023) << "\n";
            // Faut-il garder les lemmes qui ne figurent pas dans le Bailly ?
            // Oui ! Car il faudra bien vérifier les traductions données.
        }
        else
        {
            // Y a-t-il plusieurs fois la même traduction ?
            // Ça peut arriver quand il y a un renvoi et une entrée secondaire.
            if (lesRes.size() > 1)
            {
                int i = 0;
                while (i < lesRes.size() - 1)
                {
                        QString tri = lesRes[i].section("\t",1,1);
                        int j = i + 1;
                        while (j < lesRes.size())
                        {
                            if ((lesRes[j].section("\t",1,1) == tri) &&
                                (lesRes[i].section("\t",0,0).contains(">") ||
                                 lesRes[i].section("\t",0,0).contains("<") ||
                                 lesRes[j].section("\t",0,0).contains(">") ||
                                 lesRes[j].section("\t",0,0).contains("<")))
                            {
                                // Je ne supprime que si l'un des deux est un renvoi
                                if (lesRes[i].section("\t",3,3).isEmpty())
                                    lesRes[i].append(lesRes[j].section("\t",3,3));
                                lesRes.removeAt(j); // Faire des vérifs supplémentaires ?
                            }
                            else j++;
                        }
                    i++;
                }
            }
            if (nbre > 100) deb.prepend("? "); // Association douteuse
            else if (lesRes.size() > 1) deb.prepend("* "); // Plusieurs prétendants.
            for (int i = 0; i < lesRes.size(); i++)
                flux << deb << lesRes[i] << fin << nbre << "\n";
            if (freq || (lesRes.size() > 1) || (nbre > 100))
            {
                // Je voudrais faire vérifier les traductions
                // des lemmes réputés fréquents,
                // de ceux qui ont plusieurs prétendants
                // ainsi que les associations douteuses.
                QString lemmes = "";
                QString trads = "\t";
                for (int i = 0; i < lesRes.size(); i++)
                {
                    lemmes.append(lesRes[i].section("\t",0,0) + " [");
                    lemmes.append(lesRes[i].section("\t",3,3) + "]@");
                    trads.append(lesRes[i].section("\t",1,1) + "@");
                }
                lemmes.chop(1);
                lemmes.remove(" []");
                trads.chop(1);
                if (complexes.contains(uni2betacode(lem)))
                    sout3 << deb << lemmes << trads << fin2 << (numbis & 1023) << "\t" << nettoie(lem) << "\n";
                else if (deb.contains("-"))
                    sout2 << deb << lemmes << trads << fin2 << (numbis & 1023) << "\n";
                // Les composés devraient être éliminés ???
                else
                    sout << deb << lemmes << trads << fin2 << (numbis & 1023) << "\n";
            }
            else if (complexes.contains(uni2betacode(lem)))
            {
                if (lesRes[0].section("\t",3,3).isEmpty())
                                sout3 << deb << lesRes[0].section("\t",0,0) << "\t"
                                       << lesRes[0].section("\t",1,1)
                                       << fin2 << (numbis & 1023) << "\t" << nettoie(lem) << "\n";
                            else sout3 << deb << lesRes[0].section("\t",0,0) << " ["
                                       << lesRes[0].section("\t",3,3) << "]\t"
                                       << lesRes[0].section("\t",1,1)
                                       << fin2 << (numbis & 1023) << "\t" << nettoie(lem) << "\n";
            }
            else if (lesRes[0].section("\t",3,3).isEmpty())
                sout2 << deb << lesRes[0].section("\t",0,0) << "\t"
                       << lesRes[0].section("\t",1,1)
                       << fin2 << (numbis & 1023) << "\n";
            else sout2 << deb << lesRes[0].section("\t",0,0) << " ["
                       << lesRes[0].section("\t",3,3) << "]\t"
                       << lesRes[0].section("\t",1,1)
                       << fin2 << (numbis & 1023) << "\n";
        }
    }
    fichier.close();
    fout.close();
    fout2.close();
    fout3.close();
    // Je vais rouvrir le fichier des cas complexes pour séparer les groupes
    // en fonction de la fréquence de l'élément le plus fréquent.
    pseudos.clear();
    QMap<QString, int> nbPs; // Pour retenir le plus grand nombre d'occurrences.
    fout.setFileName("/Users/Philippe/Documents/Chantiers_Eulexis/complex1.csv");
    fout.open(QFile::WriteOnly | QFile::Text);
    fout2.setFileName("/Users/Philippe/Documents/Chantiers_Eulexis/complex2.csv");
    fout2.open(QFile::WriteOnly | QFile::Text);
    fout3.open(QFile::ReadOnly | QFile::Text);
    while (!sout3.atEnd()) {
        beta = sout3.readLine();
        lem = beta.section("\t",-1,-1);
        pseudos.insert(lem, beta);
        if (nbPs.contains(lem))
        {
            if (nbPs[lem] < beta.section("\t",8,8).toInt())
                nbPs[lem] = beta.section("\t",8,8).toInt();
        }
        else nbPs[lem] = beta.section("\t",8,8).toInt();
    }
    foreach (lem, nbPs.keys()) {
        complexes = pseudos.values(lem);
        for (int i = 0; i < complexes.size(); i++)
            if (nbPs[lem] < 40) sout2 << complexes[i] << "\n";
            else sout << complexes[i] << "\n";
    }
    fout.close();
    fout2.close();
    fout3.close();
}
#else
{
    QFile fichier (_rscrDir + "index_com.inc");
    fichier.open(QFile::WriteOnly | QFile::Text);
    QString lg = "<?php\n$LSJ_name = \"" + _LSJname;
    fichier.write(lg.toUtf8());
    lg = "\";\n$Pape_name = \"" + _PapeName;
    fichier.write(lg.toUtf8());
    lg = "\";\n$B_Abr_name = \"" + _AbrBaillyName;
    fichier.write(lg.toUtf8());
    lg = "\";\n$Bailly_name = \"" + _BaillyName;
    fichier.write(lg.toUtf8());
    lg = "\";\n?>\n";
    fichier.write(lg.toUtf8());
    fichier.close();
    // Je crée un fichier index_com.inc qui contient les noms de fichiers.
    // Il sera utilisé par traitement.php avec un include "data/index_com.inc".
    // Les deux fichiers index_com.inc et .csv étant créés en même temps,
    // On saura facilement qui va avec quoi.

//    fichier.setFileName("/Users/Philippe/Documents/Bailly_2020/uniques.txt");
//    fichier.open(QFile::WriteOnly | QFile::Text);
    // Lors de la construction de l'index commun, je peux regarder si une forme du Bailly existe dans les autres dicos.

    QStringList tout = _LSJindex.keys();
    tout << _AbrBaillyIndex.keys();
    tout << _PapeIndex.keys();
    tout << _BaillyIndex.keys();
    tout.sort();
    tout.removeDuplicates();
    qDebug() << tout.size();
    QMultiMap<QByteArray,QString> comIndex;
    // J'ai l'ensemble des clefs en caractères latins
    foreach (QString clef, tout) if (!clef.isEmpty())
    {
        QString clef_gr = beta2unicode(clef,false);
//        clef_gr.replace("ϐ","β");
        if (clef == "s") clef_gr = "σ";
        QStringList valLJS = _LSJindex.values(clef);
        QStringList valPape = _PapeIndex.values(clef);
        QStringList valAbrBailly = _AbrBaillyIndex.values(clef);
        QStringList valBailly = _BaillyIndex.values(clef);
        if ((valBailly.size() > 0) && ((valLJS.size() > 0) || (valPape.size() > 0)))
        for (int i = 0; i < valBailly.size(); i++)
        {
            QString b = valBailly[i].section(":",2,2);
            if (b[b.size()-1].isDigit()) b.chop(1);
            bool OK = false;
            if (b[0].isUpper()) OK = true;
            for (int j = 0; j < valLJS.size(); j++)
                if (b == valLJS[j].section(":",2,2))
                    OK = true;
            for (int j = 0; j < valPape.size(); j++)
                if (b == valPape[j].section(":",2,2))
                    OK = true;
            b = valBailly[i].section(":",2,2);
//            if (!OK) fichier.write(b.append("\n").toUtf8());
        }
        int nMax = valLJS.size();
        if (valPape.size() > nMax) nMax = valPape.size();
        if (valAbrBailly.size() > nMax) nMax = valAbrBailly.size();
        if (valBailly.size() > nMax) nMax = valBailly.size();
        for (int i=nMax; i>0;i--)
        {
            QString ligne = clef_gr + ":";
            if (i<=valLJS.size())
                ligne.append(valLJS[valLJS.size()-i].section(":",1,2)+":");
            else ligne.append("::");
            if (i<=valPape.size())
                ligne.append(valPape[valPape.size()-i].section(":",1,2)+":");
            else ligne.append("::");
            if (i<=valAbrBailly.size())
                ligne.append(valAbrBailly[valAbrBailly.size()-i].section(":",1,2)+":");
            else ligne.append("::");
            if (i<=valBailly.size())
                ligne.append(valBailly[valBailly.size()-i].section(":",1,2)+":");
            else ligne.append("::");
            if (nMax > 1)ligne.append("*\n");
            else ligne.append("\n");
            comIndex.insert(clef_gr.toUtf8(),ligne);
        }
    }
//    fichier.close();

    fichier.setFileName(_rscrDir + "index_com.csv");
    fichier.open(QFile::WriteOnly | QFile::Text);
    QByteArray ba;
    foreach (QByteArray cle, comIndex.keys())
        if (cle != ba)
        {
            ba = cle;
            foreach (lg, comIndex.values(cle))
                fichier.write(lg.toUtf8());
        }

    fichier.close();
}
#endif

/**
 * @brief verifie l'ordre des balises dans l'article
 * @param ligne : un paragraphe du fichier dictionnaire
 *
 * Lorsque je mets à jour un dictionnaire, je vérifie que chaque @a ligne
 * (c'est à dire chaque article du dictionnaire) est correctement
 * formé du point de vue de l'ouverture et de la fermeture des balises HTML.
 * Il y a un problème si on rencontre une balise fermante sans
 * que la balise ouvrante associée soit la dernière vue
 * (les paires ouvrantes-fermantes imbriquées sont éliminées au fur et à mesure)
 * ou qu'il reste des balises ouvertes à la fin de l'article.
 *
 * @todo Actuellement, j'utilise qDebug() qui affiche, dans Qt Creator,
 * les problèmes rencontrés. Il faudrait changer ça, pour que ces erreurs
 * soient affichées dans une fenêtre de dialogue qui permette aussi
 * de les sauver dans un fichier.
 */
void Lemmat::verif(QString ligne)
{
    // Je vérifie que l'ordre des balises est bon
    QStringList balises = ligne.split('>');
    QStringList listeBalises;
    bool OK = true;
    QString bal;
    for (int ii = 0; ii < balises.size(); ii++)
    {
        bal = balises[ii];
        if ((bal.count("<") == 1) && (ii < balises.size()-1))
        {
            QString bb = bal.section('<',1,1);
            if ((bb != "br") && (bb != "br/"))
            {
                if (bb.startsWith("/"))
                {
                    if (listeBalises.isEmpty())
                    {
                        OK = false;
                        qDebug() << "Pas de balise ouverte" << bal;
                    }
                    else if (!listeBalises.last().startsWith(bb.mid(1)))
                    {
                        OK = false;
                        qDebug() << "Balises incompatibles" << listeBalises.last() << bal;
                    }
                    else listeBalises.removeLast();
                }
                else listeBalises << bb;
            }
        }
        else if ((bal.count("<") != 0) || (ii != balises.size()-1))
        {
            OK = false;
            qDebug() << "Nb de < incorrect" << bal;
        }
    }
    if (!listeBalises.isEmpty())
    {
        OK = false;
        qDebug() << "Il reste des balises" << listeBalises.size() << listeBalises.last();
    }
    if (!OK) qDebug() << "$$$$" << ligne;
    if (!OK) qDebug() << " ";

}

/**
 * @brief change la langue-cible
 * @param lang : un entier (0 = Anglais ; 1 = Français ; 2 = Allemand)
 */
void Lemmat::setCible(int lang)
{
    _cible = lang;
}

/**
 * @brief accesseur de Lemmat::_cible
 * @return un entier (0 = Anglais ; 1 = Français ; 2 = Allemand)
 */
int Lemmat::cible()
{
    return _cible;
}

/**
 * @brief lecture des analyses dans le fichier "analyses_gr.txt"
 *
 * Comme le chargement prend du temps,
 * j'affiche une fenêtre avec une barre de progression.
 */
void Lemmat::lireAnalyses()
{
    _formes.clear();
//    QMap<QString,QString> flexion;
//    QMap<QString,int> preVerb;
    // Je cherche à faire un fichier le flexion : le lemme comme clef
    // et toutes les formes et analyses qui lui sont associées.
    QFile fListe (_rscrDir + "analyses_gr.txt");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    fluxL.setCodec("UTF-8");
    QString ligne;
    QString clef;
    int i = fListe.size()/100;
    int ratio = 1024;
    while (ratio < i) ratio *= 2;
    QProgressDialog progr("Chargement des analyses...", QString(), 0, fListe.size()/ratio);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    i=0;
    int j=0;
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        i += ligne.size();
        if (j != i/ratio)
        {
            j = i / ratio;
            progr.setValue(j);
        }
        clef = ligne.section("\t",0,0);
        clef.remove("(");
        clef.remove(")");
        clef.remove("/");
        clef.remove("\\");
        clef.remove("|");
        clef.remove("=");
        clef.remove("+");
        clef.remove("*");
        // j'enlève les caractères spéciaux du betacode.
        _formes.insert(clef,ligne);
        /*
         * J'essaie de construire un fichier de flexion
         *
         * */
/*        foreach (QString lg, ligne.split("}\t"))
        {
    //        fGrAn.seek(p);
    //        QString ligne = fGrAn.readLine();
            QStringList ecl = lg.split("{");
            QString mot = ecl[0].mid(0,ecl[0].size()-1);
            for (int i = 1; i<ecl.size();i++)
            {
                QStringList e = ecl[i].split("\t");
                e[1].remove("}");
                QString forme = e[0];
                QString lem = e[0];
                QString ff = forme;
                lg = "{" + mot;
                if (forme.contains("-"))
                {
                    // Si l'indication contient un "-",
                    // on trouve le lemme à sa droite
                    // et le (ou les) préverbe(s) à sa gauche,
                    // éventuellement séparés par des virgules.
                    // Mais il y a aussi des cas où la forme avec détails est donnée !
                    // 19 préverbes avec 97 141 occurrences
                    // 14 125 formes avec 22 401 occurrences
                    if (forme.contains(",") &&
                            ((forme.section(",",0,0).size() > 6) || (forme.section(",",0,0) == "kassw=")))
                    {
                        // kassw= est le seule forme de six caractères ou moins
                        // qui ne soit pas un préfixe.
                        ff = forme.section(",",0,0);
                        lem = forme.section(",",1);
                        if (ff != forme) lg.append("," + ff);
                        // Si la forme est redonnée dans l'analyse, je la remets ici.
                    }
                    else
                    {
                        // rien à faire ?
                    }
                    QString pre = lem.section("-",0,0);
                    foreach (QString p, pre.split(","))
                    {
                        if (preVerb.contains(p)) preVerb[p] += 1;
                        else preVerb.insert(p,1);
                    }
                }
                else if (forme.contains(","))
                {
                    ff = forme.section(",",0,0);
                    lem = e[0].section(",",1);
                    if (ff != forme) lg.append("," + ff);
                    // Si la forme est redonnée dans l'analyse, je la remets ici.
                }
                lg.append("\t" + e[1] + "}");
                if (flexion.contains(lem))
                {
                    // Le lemme existe déjà
                    flexion[lem].append(lg);
                }
                else flexion.insert(lem,lg);
            }
        }*/
        // Fin insertion flexion
    }
    fListe.close();
    // Sortie des flexions
/*    QMultiMap<QString,QString> sans;
    fListe.setFileName("/Users/Philippe/Documents/flexion.txt");
    fListe.open(QIODevice::WriteOnly|QIODevice::Text);
    foreach (QString lg, flexion.keys())
    {
        fluxL << lg << "\t" << flexion[lg] << "\n\n";
        ligne = lg;
        lg.remove("*");
        lg.remove("_");
//        lg.remove("-");
        // Je viens de m'apercevoir que je me suis trompé dans
        // l'interprétation du tiret et des virgules. À revoir ! le 23 mai 2020.
        // Une lemmatisation comme dia/,kata/,a)po/-ne/w doit être comprise
        // comme le lemme ne/w avec les trois préverbes devant.
        lg.remove("+");
        if (ligne.startsWith("*") && (lg.indexOf(reLettres) > 0))
        {
            // J'avais une majuscule avec esprit et, éventuellement, accent.
            int p = lg.indexOf(reLettres);
            QChar c = lg[p];
            lg.remove(p,1);
            lg.prepend(c);
        }
        if (lg[lg.size() - 1].isDigit()) lg.chop(1);
        sans.insert(lg,ligne);
    }
    fListe.close();
    fListe.setFileName("/Users/Philippe/Documents/preVerbes.csv");
    fListe.open(QIODevice::WriteOnly|QIODevice::Text);
    foreach (QString lg, preVerb.keys())
        fluxL << lg << "\t" << preVerb[lg] << "\n";
    fListe.close();

    fListe.setFileName("/Users/Philippe/Documents/doublons2.txt");
    fListe.open(QIODevice::WriteOnly|QIODevice::Text);
    QStringList ll = sans.keys();
    ll.removeDuplicates();
    foreach (ligne, ll) if (sans.values(ligne).size() > 1)
    {
        fluxL << ligne << "\t" << sans.values(ligne).size();
        foreach(QString lg, sans.values(ligne))
            fluxL << "\t" << lg;
        fluxL << "\n\n";
    }
    fListe.close();*/
}

/**
 * @brief lecture des traductions dans le fichier "trad_gr_en_fr_de.csv"
 *
 * Comme le chargement prend du temps,
 * j'affiche une fenêtre avec une barre de progression.
 */
void Lemmat::lireTraductions()
{
    _trad.clear();
    QFile fListe (_rscrDir + "trad_gr_en_fr_de.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    fluxL.setCodec("UTF-8");
    QString ligne;
    int i = fListe.size()/100;
    int ratio = 1024;
    while (ratio < i) ratio *= 2;
    QProgressDialog progr("Chargement des traductions...", QString(), 0, fListe.size()/ratio);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    i=0;
    int j=0;
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        i += ligne.size();
        if (j != i/ratio)
        {
            j = i / ratio;
            progr.setValue(j);
        }
        if (!ligne.startsWith("!"))
        {
            _trad.insert(ligne.section("\t",0,0),ligne.section("\t",1));
        }
    }
    fListe.close();
}

/**
 * @brief donne la traduction du lemme dans la langue-cible
 * @param lem : le lemme
 * @return la traduction du lemme @a lem
 *
 * Si la traduction n'existe pas dans la langue demandée,
 * j'essaie l'anglais, puis le français (si ce n'est pas déjà fait).
 *
 * Le lem que je passe ici est celui trouvé dans le fichier d'analyses :
 * il peut contenir un "-" ou/et une "," que je dois éliminer.
 * Si l'indication contient un "-",
 * on trouve le lemme à sa droite
 * et le (ou les) préverbe(s) à sa gauche,
 * éventuellement séparés par des virgules.
 * Mais il y a aussi des cas où la forme avec détails est donnée !
 * 19 préverbes avec 97 141 occurrences
 * 14 125 formes avec 22 401 occurrences
 */
QString Lemmat::traduction(QString lem)
{
    // Le lem que je passe ici est celui trouvé dans le fichier :
    // il peut contenir un "-" ou/et une "," que je dois éliminer.
    if (lem.contains("-"))
    {
        // Si l'indication contient un "-",
        // on trouve le lemme à sa droite
        // et le (ou les) préverbe(s) à sa gauche,
        // éventuellement séparés par des virgules.
        // Mais il y a aussi des cas où la forme avec détails est donnée !
        // 19 préverbes avec 97 141 occurrences
        // 14 125 formes avec 22 401 occurrences
        if (lem.contains(",") &&
                ((lem.section(",",0,0).size() > 6) || (lem.section(",",0,0) == "kassw=")))
        {
            // kassw= est le seule forme de six caractères ou moins
            // qui ne soit pas un préfixe.
            lem = lem.section(",",1); // lem est en betacode.
        }
        lem = lem.section("-",1); // lem est maintenant l'élément à traduire
    }
    else if (lem.contains(","))
    {
        // Si les infos contiennent une virgule mais pas de tiret,
        // il n'y a qu'une virgule qui sépare les détails de la forme et le lemme.
        lem = lem.section(",",1);
    }

    QString res = "";
    if (_trad.contains(lem))
    {
        res = _trad[lem].section("\t",_cible,_cible);
        if (res.isEmpty())
        {
            // La traduction demandée n'existe pas :
            // j'essaie une traduction anglaise, puis française.
            if (_cible != 0) res = "[En] " + _trad[lem].section("\t",0,0);
            // Si _cible était égale à 0, inutile de chercher une traduction au même endroit.
            if ((res == "[En] ") && (_cible != 1)) res = "[Fr] " + _trad[lem].section("\t",1,1);
            // Pour l'instant, je n'ai pas de traduction en Allemand sans traduction anglaise.
        }
    }
    return res;
}

/**
 * @brief accesseur de Lemmat::_toInit
 * @return un booléen @c true si les données n'ont pas encore été lues.
 */
bool Lemmat::toInit()
{
    return _toInit;
}

/**
 * @brief lit les fichiers d'analyse et de traduction
 *
 * Cette fonction a été reportée au moment où une lemmatisation est demandée.
 * Elle prend du temps et ne doit se faire qu'une seule fois.
 * Elle bascule donc le booléen Lemmat::_toInit à @c false.
 */
void Lemmat::initData()
{
    lireAnalyses();
    lireTraductions();
    _toInit = false;
}

/**
 * @brief cherche une traduction
 * @param bla : une chaine qui peut donner un indice
 * @return Une traduction si j'en ai trouvé une.
 *
 * @deprecated Je l'ai utilisée pour corriger les listes de traductions, mais ne sert plus.
 */
QString Lemmat::chTrad(QString bla)
{
    QString res = "";
    QString b = bla.section(" ",0,0).simplified();
    if (b.endsWith(",")) b.chop(1);
    if (b.endsWith(".")) b.chop(1);
    QString clef = uni2betacode(b);
    if (clef == b) return res; // pas du grec !
//                                qDebug() << bla << clef;
    if (!bla.section(" ",1,1).isEmpty() && bla.section(" ",1,1).at(0).isDigit())
    {
        clef.append(bla.section(" ",1,1).at(0));
        if (_trad.contains(clef))
        {
            b.append(bla.section(" ",1,1).at(0));
            res = b + "\t" + _trad[clef];
            return res;
        }
        else clef.chop(1);
    }
    if (_trad.contains(clef))
        res = b + "\t" + _trad[clef];
    else if (_trad.contains(clef+"1"))
        res = b + "1\t" + _trad[clef+"1"];
    else if (_trad.contains(clef+"2"))
        res = b + "2\t" + _trad[clef+"2"];
    else res = b + " ?\t\t\t"; // J'ai un mot grec, mais je ne l'ai pas trouvé.
    return res;
}

/**
 * @brief réparation des traductions non-trouvées ou qui peuvent sembler incomplètes
 * @param nom : nom d'un fichier contenant des traductions se terminant avec un mot outil
 *
 * @deprecated Je l'ai utilisée pour corriger les listes de traductions, mais ne sert plus.
 */
void Lemmat::repairTransl(QString nom)
{
    if (nom.isEmpty())
    {
        // Pour chercher toutes les entrées du LSJ, qui n'ont pas donné de traduction
        QFile fListe("/Users/Philippe/Documents/Chantiers_Eulexis/LSJ.csv");
        if (!fListe.open(QFile::WriteOnly | QFile::Text))
            qDebug() << "Echec";
        QTextStream fluxL(&fListe);
        fluxL.setCodec("UTF-8");
        fluxL << "\n";
//        if (_toInit) initData();
        qDebug() << "Ok";
        QString beta;
        QString tr;
        QString ligne;
        QString clef;
        QStringList LSJ;
        QStringList lost;
        int aa = 0;
        int bb = 0;
        QString nvlTrad;
        foreach (beta, _trad.keys())
        {
            tr = _trad[beta];
            if (tr.startsWith("\t"))
            {
                // Je n'ai pas de traduction anglaise
                nvlTrad = "";
                QString c = beta;
                if (c[c.size() - 1].isDigit()) c.chop(1);
                c.remove("-");
                LSJ = consLSJ(nettoie2(c));
                if (LSJ.size() > 3)
                {
                    // Les trois premières lignes sont les liens et les mots avant et après.
                    aa++;
                    bool found = false;
                    for(int i = 3; i < LSJ.size(); i++)
                    {
                        ligne = LSJ[i];
                        QString bla = ligne.section("</b>",0,0);
                        bla = bla.mid(bla.indexOf("<b>")+3); // Le lemme.
                        bool bon = (c == uni2betacode(bla)); // Condition stricte
                        bon = bon || ligne.contains(beta2unicode(beta,false)); // Si la clef est une variante.
                        if (c.lastIndexOf("(") > 2) c.remove(c.lastIndexOf("("), 1);
                        if (c.lastIndexOf(")") > 2) c.remove(c.lastIndexOf(")"), 1);
                        if (c.count("/") == 2) c.remove(c.indexOf("/"), 1);
                        bon = bon || (c == uni2betacode(bla));
                        bla = uni2betacode(bla);
                        bla.remove("+"); // Les trémas peuvent avoir échappé.
                        bon = bon || (c == bla);
                        if (bon)
                        {
                            bb++;
                            // C'est le bon lemme, peut-être.
                            found = true;
                            // Dans le cas où la traduction est vide,
                            // je prends le premier morceau en gras
                            bla = ligne.section("</span>",1).replace("&nbsp;"," ").simplified();
                            // Tout ce qui vient après le lemme
                            if (bla.contains("im. of"))
                            {
                                clef = bla.section("im. of",1).simplified();
                                if (clef.startsWith("</i>")) clef = clef.mid(4);
                                if (clef.startsWith(" ")) clef = clef.mid(1);
                                if (!clef.section(" ",1,1).isEmpty() && clef.section(" ",1,1).at(0).isDigit())
                                {
                                    QString d = clef.section(" ",1,1).mid(0,1);
                                    clef = clef.section(" ",0,0);
                                    if (clef.endsWith(",")) clef.chop(1);
                                    if (clef.endsWith(".")) clef.chop(1);
                                    clef.append(" " + d);
                                }
                                else clef = clef.section(" ",0,0);
                                if (clef.endsWith(",")) clef.chop(1);
                                if (clef.endsWith(".")) clef.chop(1);
                                if (clef != uni2betacode(clef))
                                {
                                    QString nt = "\t\t";
                                    if (_trad.contains(uni2betacode(clef)))
                                        nt = _trad[uni2betacode(clef)];
                                    QStringList ec = nt.split("\t");
                                    nvlTrad = "—\tDim. of " + clef;
                                    if (!ec[0].isEmpty() && (ec[0].size() < 20))
                                        nvlTrad.append(" (" + ec[0] + ")");
                                    nvlTrad.append("\tdim. de " + clef);
                                    if (!ec[1].isEmpty() && (ec[1].size() < 20))
                                        nvlTrad.append(" (" + ec[1] + ")");
                                    nvlTrad.append("\tdim. zu " + clef);
                                    if (!ec[2].isEmpty() && (ec[2].size() < 20))
                                        nvlTrad.append(" (" + ec[2] + ")");
                                }
                                else qDebug() << beta2unicode(beta,false) << beta << bla;
                                if (!nvlTrad.isEmpty())
                                {
                                    fluxL << beta2unicode(beta,false) << "\t" << beta << "\t";
                                    if (tr == "\t\t") // ni Français, ni Allemand
                                        fluxL << nvlTrad << "\n";
                                    else if (tr.endsWith("\t"))
                                    {
                                        // J'avais une traduction en Français.
                                        fluxL << nvlTrad.section("\t",0,1) << "\t"
                                              << tr.section("\t",1,1) << "\t"
                                              << nvlTrad.section("\t",3,3) << "\n";
                                    }
                                    else fluxL << nvlTrad.section("\t",0,1) << tr << "\n";
                                }
                            }
                            if (bla.contains("fem. of"))
                            {
                                clef = bla.section("fem. of",1).simplified();
                                if (clef.startsWith("</i>")) clef = clef.mid(4);
                                if (clef.startsWith(" ")) clef = clef.mid(1);
                                if (!clef.section(" ",1,1).isEmpty() && clef.section(" ",1,1).at(0).isDigit())
                                {
                                    QString d = clef.section(" ",1,1).mid(0,1);
                                    clef = clef.section(" ",0,0);
                                    if (clef.endsWith(",")) clef.chop(1);
                                    if (clef.endsWith(".")) clef.chop(1);
                                    clef.append(" " + d);
                                }
                                else clef = clef.section(" ",0,0);
                                if (clef.endsWith(",")) clef.chop(1);
                                if (clef.endsWith(".")) clef.chop(1);
                                if (clef != uni2betacode(clef))
                                {
                                    QString nt = chTrad(bla.section("fem. of ",1).simplified());
                                    if (nt.isEmpty()) nt = "\t\t\t";
                                    QStringList ec = nt.split("\t");
                                    nvlTrad = "—\tfem. of " + clef;
                                    if (!ec[1].isEmpty() && (ec[1].size() < 20))
                                        nvlTrad.append(" (" + ec[1] + ")");
                                    nvlTrad.append("\tfém. de " + clef);
                                    if (!ec[2].isEmpty() && (ec[2].size() < 20))
                                        nvlTrad.append(" (" + ec[2] + ")");
                                    nvlTrad.append("\tfem. zu " + clef);
                                    if (!ec[3].isEmpty() && (ec[3].size() < 20))
                                        nvlTrad.append(" (" + ec[3] + ")");
                                }
                                else qDebug() << beta2unicode(beta,false) << beta << bla;
                                if (!nvlTrad.isEmpty())
                                {
                                    fluxL << beta2unicode(beta,false) << "\t" << beta << "\t";
                                    if (tr == "\t\t")
                                        fluxL << nvlTrad << "\n";
                                    else if (tr.endsWith("\t"))
                                    {
                                        // J'avais une traduction en Français.
                                        fluxL << nvlTrad.section("\t",0,1) << "\t"
                                              << tr.section("\t",1,1) << "\t"
                                              << nvlTrad.section("\t",3,3) << "\n";
                                    }
                                    else fluxL << nvlTrad.section("\t",0,1) << tr << "\n";
                                }
                            }
                            if (bla.contains("= ") && nvlTrad.isEmpty())
                            {
                                nvlTrad = chTrad(bla.section("= ",1));
                                if (!nvlTrad.isEmpty())
                                {
                                    fluxL << beta2unicode(beta,false) << "\t" << beta << "\t";
                                    if (tr == "\t\t")
                                        fluxL << nvlTrad << "\n";
                                    else if (tr.endsWith("\t"))
                                    {
                                        // J'avais une traduction en Français.
                                        fluxL << nvlTrad.section("\t",0,1) << "\t"
                                              << tr.section("\t",1,1) << "\t"
                                              << nvlTrad.section("\t",3,3) << "\n";
                                    }
                                    else fluxL << nvlTrad.section("\t",0,1) << tr << "\n";
                                }
                            }
                            if (bla.contains("i> for ") && nvlTrad.isEmpty())
                            {
                                nvlTrad = chTrad(bla.section("i> for ",1));
                                if (!nvlTrad.isEmpty())
                                {
                                    fluxL << beta2unicode(beta,false) << "\t" << beta << "\t> ";
                                    if (tr == "\t\t")
                                        fluxL << nvlTrad << "\n";
                                    else if (tr.endsWith("\t"))
                                    {
                                        // J'avais une traduction en Français.
                                        fluxL << nvlTrad.section("\t",0,1) << "\t"
                                              << tr.section("\t",1,1) << "\t"
                                              << nvlTrad.section("\t",3,3) << "\n";
                                    }
                                    else fluxL << nvlTrad.section("\t",0,1) << tr << "\n";
                                }
                            }
                            if (bla.contains("v. sub ") && nvlTrad.isEmpty())
                            {
                                nvlTrad = chTrad(bla.section("v. sub ",1));
                                if (!nvlTrad.isEmpty())
                                {
                                    fluxL << beta2unicode(beta,false) << "\t" << beta << "\tv. ";
                                    if (tr == "\t\t")
                                        fluxL << nvlTrad << "\n";
                                    else if (tr.endsWith("\t"))
                                    {
                                        // J'avais une traduction en Français.
                                        fluxL << nvlTrad.section("\t",0,1) << "\t"
                                              << tr.section("\t",1,1) << "\t"
                                              << nvlTrad.section("\t",3,3) << "\n";
                                    }
                                    else fluxL << nvlTrad.section("\t",0,1) << tr << "\n";
                                }
                            }
                            // Fin du morceau pour traductions vides.
                        }
                    }
                    if (!found)
                        lost.append(beta2unicode(beta,false) + "\t" + beta + "\tPas exact dans le LSJ");
                    // Pas trouvé ! Je ne fais rien.
                    else if (nvlTrad.isEmpty())
                        lost.append(beta2unicode(beta,false) + "\t" + beta +"\tPas de renvoi");
                    else
                        lost.append(beta2unicode(beta,false) + "\t" + beta +"\t" + nvlTrad.section("\t",0,0));
                    // Je pense avoir trouvé l'article, mais rien en gras
                }
                else lost.append(beta2unicode(beta,false) + "\t" + beta + "\tAbsent du LSJ");
                // Pas trouvé ! Je ne fais rien.
            }
        }
        fListe.close();
qDebug() << aa << bb << lost.size();
        fListe.setFileName("/Users/Philippe/Documents/Chantiers_Eulexis/LSJ0.csv");
        fListe.open(QFile::WriteOnly | QFile::Text);
        for (int i = 0; i < lost.size() ; i++)
        {
            fluxL << lost[i] << "\n";
        }
        fListe.close();
        return;
    }
    // Cette routine est faite pour réparer les traductions qui se terminent avec un mot outil.
    // ou dans un premier temps qui se limitent à "of".
    QFile fListe(nom);
    fListe.open(QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL(&fListe);
    fluxL.setCodec("UTF-8");
    QString ligne;
    QString clef;
    QStringList LSJ;
    QStringList lost;
    QMap<QString,QString> repare;
    while (!fluxL.atEnd ())
    {
        clef = fluxL.readLine ();
        if (!clef.startsWith("!") && !clef.isEmpty())
        {
//            ligne = traduction(clef);
            QString c = clef;
            if (c.right(1)[0].isDigit()) c.chop(1);
            c.remove("-");
            LSJ = consLSJ(nettoie2(c));
            if (LSJ.size() > 3)
            {
                // Les trois premières lignes sont les liens et les mots avant et après.
                bool found = false;
                for(int i = 3; i < LSJ.size(); i++)
                {
                    ligne = LSJ[i];
                    QString bla = ligne.section("</b>",0,0);
                    bla = bla.mid(bla.indexOf("<b>")+3); // Le lemme.
                    bool bon = (c == uni2betacode(bla)); // Condition stricte
                    bon = bon || ligne.contains(beta2unicode(clef)); // Si la clef est une variante.
                    if (c.lastIndexOf("(") > 2) c.remove(c.lastIndexOf("("), 1);
                    if (c.lastIndexOf(")") > 2) c.remove(c.lastIndexOf(")"), 1);
                    if (c.count("/") == 2) c.remove(c.indexOf("/"), 1);
                    bon = bon || (c == uni2betacode(bla));
                    bla = uni2betacode(bla);
                    bla.remove("+"); // Les trémas peuvent avoir échappé.
                    bon = bon || (c == bla);
                    if (bon)
                    {
                        // C'est le bon lemme, peut-être.
                        found = true;
                        // Dans le cas où la traduction est vide,
                        // je prends le premier morceau en gras
                        bla = ligne.section("</span>",1); // Tout ce qui vient après le lemme
                        if (bla.contains("<b>"))
                        {
                            bla = bla.section("<b>",1,1);
                            bla = bla.section("</b>",0,0);
                            repare.insert(clef,bla);
                        }
                        else if (LSJ.size() == 4)
                        {
                            lost.append(clef+"\tPas de gras");
                            repare.insert(clef,"");
                            // Je pense avoir trouvé l'article, mais rien en gras
                        }
                        // Fin du morceau pour traductions vides.

/* Le morceau qui suit était là pour compléter une traduction existante
                        int p = ligne.indexOf("<b>" + traduction(clef));
                        // C'est bien : la traduction que j'ai commence une partie en gras.
                        if (p > 0) p += 3; // C'est parfait !
                        else
                        {
                            // La traduction que j'ai ne commence pas avec la balise <b>
                            p = ligne.indexOf(traduction(clef));
                            if (p > 0)
                            {
                                QString toto = ligne.mid(0,p);
                                if ((toto.lastIndexOf("<b>") > 0))
                                {
                                    toto = toto.mid(toto.lastIndexOf("<b>"));
                                    if (toto.contains("</b>")) p = -2;
                                }
                                else // pas en gras
                                {
                                    p = -2;
                                }
                            }
                        }
                        if (p > 0)
                        {
                            // C'est encourageant !
                            bla = ligne.mid(p);
                            if (traduction(clef) != bla.mid(0, bla.indexOf("</b>")))
                                repare.insert(clef,bla.mid(0, bla.indexOf("</b>")));
                            else if (bla.section("</b>",0,1).endsWith("for") ||
                                     bla.section("</b>",0,1).endsWith("from") ||
                                     bla.section("</b>",0,1).endsWith("on"))
                            {
                                // Prendre un mot de plus ?
                                QString fin = bla.section("</b>",2).trimmed();
                                bla = bla.section("</b>",0,1);
                                if (fin.startsWith(",")) fin = fin.mid(1).trimmed();
                                if (fin.startsWith("or"))
                                {
                                    fin = fin.section("</b>",0,0);
                                    bla.append(" " + fin);
                                }
                                else bla.append(" " + fin.section(" ",0,0));
                                bla.remove("<b>");
                                bla.remove("</b>");
                                if (bla.contains("<a href"))
                                    bla = bla.section("<a href",0,0).trimmed();
                                repare.insert(clef,bla);
                            }
                            else
                            {
                                bla = bla.section("</b>",0,1);
                                bla.remove("<b>");
                                bla.remove("</b>");
                                if (bla.contains("<a href"))
                                    bla = bla.section("<a href",0,0).trimmed();
                                repare.insert(clef,bla);
                            }
                        }
                        else if (LSJ.size() == 4)
                        {
                            if (p == -1)
                            {
                                lost.append(clef+"\tPas trouvé la trad.");
                                repare.insert(clef,"");
                            }
                            else
                            {
                                lost.append(clef+"\tPas en gras");
                                repare.insert(clef,"");
                            }
                        }*/
                    }
                }
                if (!found) lost.append(clef+"\tPas exact dans LSJ");
            }
            else lost.append(clef+"\tAbsent du LSJ");
            // Pas trouvé ! Je ne fais rien.
        }
    }
    fListe.close();
    nom.chop(3);
    nom.append("csv");
    fListe.setFileName(nom);
    fListe.open (QFile::WriteOnly | QFile::Text);
    foreach (ligne, repare.keys())
    {
        fluxL << beta2unicode(ligne) << "\t" << ligne << "\t"
              << traduction(ligne) << "\t" << repare[ligne] << "\n";
    }
    fListe.close();

    nom.chop(4);
    nom.append("_lost.csv");
    fListe.setFileName(nom);
    fListe.open (QIODevice::WriteOnly|QIODevice::Text);
    for (int i = 0; i < lost.size() ; i++)
    {
        fluxL << lost[i] << "\n";
    }
    fListe.close();
}
