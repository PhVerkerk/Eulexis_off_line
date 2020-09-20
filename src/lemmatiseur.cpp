#include "lemmatiseur.h"

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

void Lemmat::lireData()
{
//    lireAnalyses();
//    lireTraductions();
// qDebug() << _formes.size() << _trad.size() << _beta.size() << _uni.size();
    lireLSJ();
    lireAbrBailly();
    lireBailly();
    lirePape();
}

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
 * @brief Lemmat::lem2csv
 * @param f : la forme à lemmatiser
 * @param beta : pour avoir des beta intérieurs dans les formes
 * @return Une QStringList avec les différents lemmes possibles
 *
 * Il s'agit de reprendre la routine lemmatise ci-dessus,
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

QString Lemmat::uni2betacode(QString f)
{
    // Transf l'unicode en betacode
    for (int i=0; i<_beta.size();i++)
        f.replace(_uni[i],_beta[i]);
//    for (int i=0; i<f.size(); i++)
  //      if (f[i].unicode() > 127) qDebug() << f[i] << f;
    return f;
}

QString Lemmat::nettoie(QString f)
{
    f.remove("\u1FBF");
    // Dans le Bailly, il y a un rho majuscule avec un esprit doux qui n'existe pas en Unicode
    // d'où l'utilisation de ce caractère "psili" tout seul (dans deux mots).
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
        fli << "! Le Bailly 2020 a été mis en TeX par Gérard Gréco et son équipe.\n";
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
                cle = linea.section("'>",0,0);
                cle = cle.section("'",-1);
                cle = nettoie(cle).toLower();
                listClefs.append(cle.split(", "));
                listClefs.removeDuplicates();
            }

//            fli << cle << "\n" << clef << ":" << p << ":" << cle << ":" << cle_prec << ":";
            // La ligne est incomplète : la clé suivante viendra la compléter.
        }
//        fli << QString("Α\n"); // Compléter la dernière ligne avec le premier mot.
    }
    else qDebug() << "erreur";
    fandr.close ();
    findex.close ();
    lireBailly();
}

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
    QStringList ll = _BaillyIndex.keys();
    ll.removeDuplicates();
    findex.setFileName("/Users/Philippe/Documents/Bailly_cnt.csv");
    findex.open(QFile::WriteOnly | QFile::Text);
    foreach (QString bla, ll)
    {
        QString b = bla + ":%1\n";
        b = b.arg(_BaillyIndex.values(bla).size());
        findex.write(b.toUtf8());
    }
    findex.close();
}

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

void Lemmat::indexCommun()
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

    fichier.setFileName("/Users/Philippe/Documents/Bailly_2020/uniques.txt");
    fichier.open(QFile::WriteOnly | QFile::Text);
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
            if (!OK) fichier.write(b.append("\n").toUtf8());
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
    fichier.close();

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

void Lemmat::setCible(int lang)
{
    _cible = lang;
}

int Lemmat::cible()
{
    return _cible;
}

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

QString Lemmat::traduction(QString lem)
{
    return _trad[lem].section("\t",_cible,_cible);
}

bool Lemmat::toInit()
{
    return _toInit;
}

void Lemmat::initData()
{
    lireAnalyses();
    lireTraductions();
    _toInit = false;
}

void Lemmat::repairTransl(QString nom)
{
    // Cette routine est faite pour réparer les traductions qui se terminent avec un mot outil.
    // ou dans un premier temps qui se limitent à "of".
    QFile fListe (nom);
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
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
    fListe.open (QIODevice::WriteOnly|QIODevice::Text);
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
