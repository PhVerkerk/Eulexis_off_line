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
    lireBailly();
    lirePape();*/
    reLettres = QRegExp("[A-Za-z]");
    rePonct = QRegExp("([\\.?!;:,\\)\\(])");
}

void Lemmat::lireData()
{
    lireAnalyses();
    lireTraductions();
qDebug() << _formes.size() << _trad.size() << _beta.size() << _uni.size();
    lireLSJ();
    lireBailly();
    lirePape();
}

QStringList Lemmat::lemmatise(QString f,bool beta)
{
    QString f_gr = "";
    if (!f.contains(reLettres))
    {
        f_gr = f; // Je garde la forme grecque.
        f = nettoie(f_gr);
/*        if (!beta) f_gr.replace("ϐ","β");
        else
        {
            f_gr.replace("β","ϐ");
            if (f_gr.startsWith("ϐ"))
                f_gr = "β" + f_gr.mid(1);
            // La forme grecque sera comparée au résultat de la conversion beta2unicode.
            // Je dois donc mettre ou enlever les beta intérieurs.
        } */
        f_gr = uni2betacode(f_gr);
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
//        llem << "Forme non trouvée / Not found";
        return llem;
    }
//    QFile fGrAn (_rscrDir + "greek-analyses.txt");
//    fGrAn.open (QIODevice::ReadOnly|QIODevice::Text);
    QString analyses = _formes.value(f);
//    foreach (qint64 p, _formes.values(f))
    foreach (QString ligne, analyses.split("}\t"))
    {
//        fGrAn.seek(p);
//        QString ligne = fGrAn.readLine();
        QStringList ecl = ligne.split("{");
        QString mot = ecl[0].mid(0,ecl[0].size()-1);
        ligne = beta2unicode(mot,beta);
        if (mot == f_gr)
        {
            ligne.prepend("<span style='color:red'>");
            ligne.append("</span>");
        }
        ligne.append("<ul>");
        for (int i = 1; i<ecl.size();i++)
        {
            QStringList e = ecl[i].split("\t");
            e[1].remove("}");
            QString forme = beta2unicode(e[0],beta);
            QString lem = e[0];
            QString ff = forme;
            if (forme.contains(","))
            {
                ff = forme.section(",",0,0);
                lem = e[0].section(",",1);
                lem.remove(",");
                forme.replace("σ,","ς,");
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
            llem.prepend(ligne + "</ul>");
        else llem << ligne + "</ul>";
    }
    return llem;
}

QString Lemmat::beta2unicode(QString f, bool beta)
{
    // Transf le betacode en unicode
    if (f.endsWith("s"))
    {
        f.chop(1);
        f.append("ς");
        // Le sigma final
    }
//    qDebug() << f;
    if (!beta) f.replace("b","β");
    else if (f.startsWith("b"))
        f = "β" + f.mid(1);
//    qDebug() << beta << f;
    if (f[f.size()-1].isDigit() && (f[f.size()-2]=='s'))
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
    for (int i=0; i<f.size(); i++)
        if (!f[i].unicode() > 127) qDebug() << f[i] << f;
    return f;
}

QString Lemmat::nettoie(QString f)
{
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

void Lemmat::majBailly(QString nom)
{
    // Pour le dico "XMLBailly*.txt"
    QFile fandr (_rscrDir + nom);
    QFile findex (_rscrDir + "Bailly.csv");
    QString cle_prec="ὤκιμον"; // dernier mot du Bailly
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
        fli << "! Philippe Verkerk 2017.\n!\n";
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
            // Pour le Bailly, le premier mot avant un double blanc
//            cle = linea.section("  ",0,0).trimmed();
            cle = linea.section("\t",0,0).trimmed();
            cle.replace("ϐ","β"); // Le Bailly est le seul à utiliser le ϐ intérieur.
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
        fli << "! Philippe Verkerk 2017.\n!\n";
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
    else qDebug() << _LSJname;
    QProgressDialog progr("Chargement du LSJ...", "Arrêter", 0, findex.size());
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        progr.setValue(findex.pos());
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
    qDebug() << _LSJindex.size();

    // Je lis aussi la liste des références aux œuvres.
    findex.setFileName(_rscrDir + "Ref_LSJ.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString f = "@%1@";
    // $ref_apres[$x_ref] = "<a href='Liste_Auteurs_LSJ.htm#" . $eclats[1] . "' title='" . $eclats[2] . "'>" . $eclats[0] . "</a>";
    QString href = "<a href='@%1'>%2</a>";
    // Pour que les références les plus longues passent d'abord !
//    QMultiMap<int,QString> ordre;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
        QStringList eclats = linea.split("\t");
        eclats[4].replace("'","’");
        eclats[4].replace("<","≤");
        eclats[4].replace(">","≥");
        _refLSJ << eclats[0];
        _tmpLSJ << f.arg(_refLSJ.size()-1);
        _renLSJ << href.arg(eclats[4]).arg(eclats[0]);
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

void Lemmat::lireBailly()
{
    // Charger en mémoire l'index du Bailly
    if (!QFile::exists(_rscrDir + "Bailly.csv")) return;
    _BaillyIndex.clear();
    QFile findex (_rscrDir + "Bailly.csv");
    findex.open(QFile::ReadOnly | QFile::Text);
    QString linea = findex.readLine();
    _BaillyName = linea.mid(1).trimmed();
    if (_BaillyName.isEmpty()) qDebug() << "Erreur : le nom du Bailly manque";
    else qDebug() << _BaillyName;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
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
    qDebug() << _BaillyIndex.size();
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
    else qDebug() << _PapeName;
    while (!findex.atEnd ())
    {
        linea = findex.readLine();
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
    qDebug() << _PapeIndex.size();
}

QStringList Lemmat::consLSJ(QString f)
{
    // Consulter le LSJ pour la forme f
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
    QStringList llem = _LSJindex.values(f);
    if (llem.isEmpty()) return llem;
    if (f_gr != "")
    {
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
    QStringList res = consult(_LSJname,llem,"L");
    // Je fais du post-traitement : recherche des renvois.
    if (!res.isEmpty()) for (int i=3; i<res.size();i++)
    {
        // Les 3 premiers éléments donnent le mot avant, le mot après et les liens.
        QString article = res[i];
        article.replace("\t"," "); // le tab
        article.replace("\u00a0"," "); // l'espace insécable
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

QStringList Lemmat::consBailly(QString f)
{
    // Consulter le Bailly pour la forme f
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
    QStringList llem = _BaillyIndex.values(f);
    if (llem.isEmpty()) return llem;
    if (f_gr != "")
    {
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
    QStringList res = consult(_BaillyName,llem,"B");
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
//        qDebug() << article;
        int p = article.indexOf("<i><b>Moy.</b></i>");
        if (article.mid(p-1,1) != "\n") article.insert(p,"<br />\n");
        article.replace("\t"," "); // le tab
        if (article.contains("v.") || article.contains("c.") || article.contains("p."))
        {
            // Il y a probablement des renvois
            qDebug() << article;
            article = lierRenvois(article, " v.");
            article = lierRenvois(article, " c.");
            article = lierRenvois(article, " p.");
            article = lierRenvois(article, ">p.</i> ");
            article = lierRenvois(article, ">v.</i> ");
            article = lierRenvois(article, ">c.</i> ");
            article = lierRenvois(article, ">p.</i>&nbsp;");
            article = lierRenvois(article, ">v.</i>&nbsp;");
            article = lierRenvois(article, ">c.</i>&nbsp;");
            qDebug() << article;
        }
        res[i] = article;
    }
    return res;
}

QStringList Lemmat::consPape(QString f)
{
    // Consulter le Pape pour la forme f
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
    QStringList llem = _PapeIndex.values(f);
    if (llem.isEmpty()) return llem;
    if (f_gr != "")
    {
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
    QStringList res = consult(_PapeName,llem,"P");
    // Je fais du post-traitement : recherche des renvois.
    if (!res.isEmpty()) for (int i=3; i<res.size();i++)
    {
        // Les 3 premiers éléments donnent le mot avant, le mot après et les liens.
        QString article = res[i];
        article.replace("\t"," "); // le tab
        article.replace("\u00a0"," "); // l'espace insécable
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
    QString div = "<div id='#" + prefix + "%1'>";
    QString aHref = "<a href='§" + prefix + "%1'>%2</a> ";
    // Normalement, c'est un # pour aller à une ancre.
    // Mais il y a un conflit avec les "#1" etc... utilisés en betacode.
    if (!QFile::exists(_rscrDir + nom)) return liens;
    QFile fichier (_rscrDir + nom);
    fichier.open(QFile::ReadOnly | QFile::Text);

    for (int i = llem.size() - 1; i > -1; i--)
    {
        QStringList ecl = llem[i].split(":");
        qint64 p = ecl[1].toInt();
        fichier.seek(p);
        QString linea = fichier.readLine();
//        qDebug() << llem[i] << linea;
        ici.append(ecl[2]);
        avant.append(ecl[3]);
        apres.append(ecl[4]);
        liens.append(aHref.arg(i).arg(ecl[2]));

        linea.insert(ecl[2].size(),"</b></span>");
        linea.prepend("<span style='color:red'><b>");
        linea.prepend(div.arg(i));
        linea.append("</div>");
        donnees.append(linea);
    }
    fichier.close();
    qDebug() << avant << apres;
    for (int i = llem.size() - 1; i > -1; i--)
    {
        if (ici.contains(avant[i])) avant.removeAt(i);
        if (ici.contains(apres[i])) apres.removeAt(i);
        // J'élimine les mots qui sont déjà affichés.
    }
    qDebug() << avant << apres;
    QString prems = avant[0];
    if (avant.size() > 1) for (int i=1; i<avant.size();i++)
        if (prems < avant[i]) prems = avant[i];
    QString der = apres[0];
    if (apres.size() > 1) for (int i=1; i<apres.size();i++)
        if (der > apres[i]) der = apres[i];
    QString lgLiens = "<big>" + liens.join(" ") + "</big>";
    lgLiens.prepend(" <a href='http://aller/" + uni2betacode(prems) + "'>" + prems + "</a> ");
    lgLiens.append(" <a href='http://aller/" + uni2betacode(der) + "'>" + der + "</a> ");
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
        fli << "! Philippe Verkerk 2017.\n!\n";
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
    QStringList tout = _LSJindex.keys();
    tout << _BaillyIndex.keys();
    tout << _PapeIndex.keys();
    tout.sort();
    tout.removeDuplicates();
    qDebug() << tout.size();
    QMultiMap<QByteArray,QString> comIndex;
    // J'ai l'ensemble des clefs en caractères latins
    foreach (QString clef, tout) if (!clef.isEmpty())
    {
        QString clef_gr = beta2unicode(clef,false);
//        clef_gr.replace("ϐ","β");
        QStringList valLJS = _LSJindex.values(clef);
        QStringList valPape = _PapeIndex.values(clef);
        QStringList valBailly = _BaillyIndex.values(clef);
        int nMax = valLJS.size();
        if (valPape.size() > nMax) nMax = valPape.size();
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
            if (i<=valBailly.size())
                ligne.append(valBailly[valBailly.size()-i].section(":",1,2)+":");
            else ligne.append("::");
            ligne.append("\n");
            comIndex.insert(clef_gr.toUtf8(),ligne);
        }
    }
    QFile fichier (_rscrDir + "index_com.csv");
    fichier.open(QFile::WriteOnly | QFile::Text);
    QByteArray ba;
    foreach (QByteArray cle, comIndex.keys())
        if (cle != ba)
        {
            ba = cle;
            foreach (QString lg, comIndex.values(cle))
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
            if (bb != "br")
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
    _formes.clear();;
    QFile fListe (_rscrDir + "analyses_gr.txt");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    fluxL.setCodec("UTF-8");
    QString ligne;
    QString clef;
    QProgressDialog progr("Chargement des données...", "Arrêter", 0, fListe.size()/8192);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);
    int i=0;
    int j=0;
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        i += ligne.size();
        if (j != i/8192)
        {
            j = i / 8192;
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
    }
    fListe.close();

}

void Lemmat::lireTraductions()
{
    _trad.clear();;
    QFile fListe (_rscrDir + "trad_gr_en_fr_de.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    fluxL.setCodec("UTF-8");
    QString ligne;
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (!ligne.startsWith("!"))
        {
            _trad.insert(ligne.section("\t",0,0),ligne.section("\t",1));
        }
    }
    fListe.close();
}
