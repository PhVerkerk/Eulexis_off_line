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

private:
    QString _rscrDir;
    QMap<QString,QString> _formes;
    QMap<QString,QString> _trad;
    int _cible; // Choix de la langue 0 = Anglais ; 1 = Français.
    QMultiMap<QString,QString> _LSJindex;
    QMultiMap<QString,QString> _PapeIndex;
    QMultiMap<QString,QString> _AbrBaillyIndex;
    QMultiMap<QString,QString> _BaillyIndex;
    QString _LSJname;
    QString _PapeName;
    QString _AbrBaillyName;
    QString _BaillyName;
    QStringList _beta;
    QStringList _uni;
    QRegExp rePonct;
    QRegExp reLettres;
    QString lierRenvois(QString article, QString renvoi);
    void verif(QString ligne);
    QStringList _refLSJ;
    QStringList _tmpLSJ;
    QStringList _renLSJ;
    bool _toInit; // Un booléen pour me dire qu'il faut encore initialiser les analyses et traductions.
    int _maxList; // Pour choisir à un seul endroit, le nombre max de réponses aux caractères de substitution.
};

#endif // LEMMAT_H
