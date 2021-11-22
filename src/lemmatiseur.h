#ifndef LEMMAT_H
#define LEMMAT_H

#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QMap>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QProgressDialog>

/**
 * @file lemmatiseur.h
 * @brief header pour la classe Lemmat
 */

/**
 * @brief La classe Lemmat regroupe les fonctions nécessaires
 * à la lemmatisation et à la consultation des dictionnaires.
 *
 * Il n'y a ici ni module de scansion ou ni tagueur.
 * Il n'y a donc pas lieu de séparer un noyau de lemmatisation des autres outils
 * qu'ils soient la lemmatisation d'un texte ou la scansion.
 * La consultation des dictionnaires pourrait être séparée de la lemmatisation,
 * mais il reste des outils communs comme la conversion entre unicode et betacode.
 *
 * Dans Eulexis, la lemmatisation se distingue radicalement de la lemmatisation
 * de Collatinus dans son principe même.
 * Ici, j'utilise une liste de formes prise, avec l'autorisation de l'auteur,
 * dans [Diogenes](https://d.iogen.es/d/), liste elle-même tirée de Perseus.
 * Pour pouvoir afficher les traductions dans d'autres langues que l'anglais,
 * j'ai séparé le lien entre la forme et son lemme du lien entre le lemme
 * et ses traductions.
 * J'ai donc deux fichiers :
 * * "analyses_gr.txt" qui lient les formes avec les lemmes (et les analyses associées)
 * * "trad_gr_en_fr_de.csv" qui lient les lemmes grecs avec des traductions en
 * anglais, français et allemand.
 *
 * Les traductions en français et en allemand ont d'abord été obtenues
 * par une traduction automatique de l'anglais. Toutefois, beaucoup de
 * corrections ont été faites depuis.
 * De plus, l'arrivée du Bailly, numérisé par Gérard Gréco, m'a permis
 * d'enrichir les traductions françaises.
 * Dans cette version d'Eulexis, il y a une interface spéciale
 * pour vérifier les traductions anglaises, françaises et allemandes
 * de tous les lemmes. Cela répond à une collaboration avec
 * [Alpheios](https://github.com/DigiClass/alpheios-french-dictionary)
 * qui, pour l'instant, n'a pas porté ses fruits.
 *
 * @note Eulexis ne sait consulter que des dictionnaires numériques
 * en HTML et non-compressés. Ils peuvent donc facilement être corrigés
 * et mis à jour par l'utilisateur.
 * @note Collatinus a la possibilité d'afficher des dictionnaires en djvu.
 * Il existe des dictionnaires de grec qui ne sont qu'en mode image,
 * par exemple le "Greek lexicon of the Roman and Byzantine periods"
 * de E. A. Sophocles qui date de 1914.
 * Je réfléchis à un outil dédié aux dictionnaires (sans lemmatisation)
 * qui permettrait de consulter tous ces ouvrages, qu'ils
 * soient numériques (compressés ou pas) ou en mode image (djvu ou pdf).
 */
class Lemmat
{
public:
    Lemmat(QString rep);
    void lireData();
    QStringList lemmatise(QString f, bool beta = true);
    QStringList lem2csv(QString f, bool beta = true);
    QString beta2unicode(QString f,bool beta = true);
    QString uni2betacode(QString f);
    QString nettoie(QString f);
    QString nettoie2(QString res);
    void majLSJ(QString nom);
    void lireAbrBailly();
    void lireBailly();
    void lireLSJ();
    void lirePape();
    QStringList consLSJ(QString f);
    QStringList consAbrBailly(QString f);
    QStringList consBailly(QString f);
    QStringList consPape(QString f);
    QStringList consAsterisk(QString f, QMultiMap<QString,QString> * dicIndex);
    QStringList consRegExp(QString f, QMultiMap<QString,QString> * dicIndex);
    QStringList cherchIndex(QString f, QMultiMap<QString,QString> * dicIndex);
    void majPape(QString nom);
    void majAbrBailly(QString nom);
    void majBailly(QString nom);
    void majAnalyses(QString nom);
    QStringList consult(QString nom, QStringList llem, QString prefix);
    void indexCommun();
    QString traduction(QString lem);
    void setCible(int lang);
    int cible();
    void lireAnalyses();
    void lireTraductions();
    bool toInit();
    void initData();
    void repairTransl(QString nom);
    QString chTrad(QString bla);
    QString voyelles = "aehiouw";
    QString reconcil(QString beta);

private:
    QString _rscrDir; /*!< Chemin complet pour le répertoire des ressources */
    QMap<QString,QString> _formes; /*!< Liste des formes*/
    QMap<QString,QString> _trad; /*!< Liste des traductions anglaises, françaises et allemandes*/
    int _cible; /*!< Choix de la langue 0 = Anglais ; 1 = Français ; 2 = Allemand.*/
    QMultiMap<QString,QString> _LSJindex; /*!< Index du LSJ*/
    QMultiMap<QString,QString> _PapeIndex; /*!< Index du Pape*/
    QMultiMap<QString,QString> _AbrBaillyIndex; /*!< Index de l'abrégé du Bailly*/
    QMultiMap<QString,QString> _BaillyIndex; /*!< Index du Bailly*/
    QString _LSJname; /*!< Nom du fichier contenant le LSJ*/
    QString _PapeName; /*!< Nom du fichier contenant le Pape*/
    QString _AbrBaillyName; /*!< Nom du fichier contenant l'abrégé du Bailly*/
    QString _BaillyName; /*!< Nom du fichier contenant le Bailly*/
    QStringList _beta; /*!< Liste des caractères grecs en betacode pour la conversion en unicode*/
    QStringList _uni; /*!< Liste des caractères grecs en unicode pour la conversion en betacode*/
    QRegExp rePonct; /*!< Expression rationnelle pour la ponctuation*/
    QRegExp reLettres; /*!< Expression rationnelle pour les lettres*/
    QString lierRenvois(QString article, QString renvoi);
    void verif(QString ligne);
    QStringList _refLSJ;
    QStringList _tmpLSJ;
    QStringList _renLSJ;
    bool _toInit; /*!< Un booléen pour me dire qu'il faut encore initialiser les analyses et traductions.*/
    int _maxList; /*!< Pour choisir à un seul endroit, le nombre max de réponses aux caractères de substitution.*/
};

#endif // LEMMAT_H
