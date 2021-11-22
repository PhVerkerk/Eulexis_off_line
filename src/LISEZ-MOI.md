
## Outil de lemmatisation du grec ancien

Eulexis est un logiciel de **lemmatisation de textes en grec ancien**, 
libre et gratuit, disponible pour Mac OS et Windows.
Cette application est développée par Philippe Verkerk.
Elle est mise à disposition sous licence GNU GPL v3,
sans aucune garantie, mais avec l'espoir qu'elle vous sera utile, 
et reste soumise à corrections et améliorations. 
Si vous remarquez des erreurs ou des coquilles, n'hésitez pas à 
nous les signaler à l'adresse eulexis@biblissima-condorcet.fr !

### Nouveautés de la version 1.2

* Les traductions ont été quelque peu améliorées, même si c'est encore perfectible.
* Des boutons de navigation permettent maintenant de revenir à un article de dictionnaire déjà consulté.
* Quelques milliers de corrections ont été reportés dans les différents dictionnaires qui existaient déjà dans Eulexis.
* Le Bailly 2020 _Hugo Chávez_ de Gérard Gréco a été ajouté aux trois dictionnaires existants.
* La consultation peut se faire avec des expressions rationnelles ou plus simplement en utilisant des caractères de substitution.
* Le démarrage de l'application est plus rapide tant qu'on consulte les dictionnaires (le chargement des analyses reste long).
* Un fichier peut être lemmatisé directement, produisant un fichier CSV avec toutes les lemmatisations connues des formes du texte (avec la traduction). Pour fournir la liste du vocabulaire associée à un texte, l'enseignant n'aura qu'à supprimer les lemmes qui ne conviennent pas (éventuellement, retoucher les traductions et supprimer les lemmatisations des mots très courants, donc déjà connus des étudiants).
* Le TextiColor peut permettre de détecter des coquilles ou des fautes d'OCR : si un mot n'est pas reconnu, il passera en gras et en rouge. Attention, ce n'est pas parce que le mot est reconnu qu'il est juste et, inversement, un mot non-reconnu peut être parfaitement légitime.

### Crédits

Un grand merci à Philipp Roelli, André Charbonnet, Mark De Wilde, Gérard Gréco, Peter J. Heslin, Yves Ouvrard, Eduard Frunzeanu et Régis Robineau.

* Le LSJ est de Philipp Roelli, revu et corrigé par Chaeréphon (André Charbonnet)
* Le Pape est de Philipp Roelli. Il a été revu par Chaeréphon (André Charbonnet) et Jean-Paul Woitrain.
* L'abrégé du Bailly est de Chaeréphon (André Charbonnet)
* Le Bailly 2020 Hugo Chávez est de Gérard Gréco, converti de TeX en HTML par Philippe Verkerk.
* La lemmatisation et la flexion ont été possibles grâce aux fichiers de Diogenes et de Perseus.
* L'amélioration des traductions anglaises a été, en partie, possible grâce au travail d'Helma Dik (Logeion).




### Mise à jour 2020-01-18

## Liste de vocabulaire et correction OCR

Suite à une demande de David Carter,
j'ai introduit la possibilité de générer
une liste de vocabulaire au format CSV 
à partir de n'importe quel texte grec.
C'est juste un nouvel élément dans le menu Fichier, appelé _txt2csv_.
Il ouvre une fenêtre de dialogue pour ouvrir un fichier texte
(avec extension **txt**)
et produira un fichier CSV du même nom
(mais avec l'extension **csv**).
Si le fichier de destination existe déjà,
il sera écrasé sans avertissement.

**NB** : il n'est pas nécessaire d'ouvrir le texte 
dans la fenêtre de texte d'Eulexis
et de le lemmatiser. Tout se fait de fichier à fichier.

Cet outil _txt2csv_ a deux options supplémentaires :

* TextiColor

* BOM4ms, cette option ajoute un marqueur d'ordre d'octet
   (pseudo-caractère Unicode) au début du fichier CSV
   pour faciliter certains produits Microsoft à comprendre
   que l'encodage des caractères est UTF-8.

### Liste de vocabulaire

Le fichier CSV produit par _txt2csv_ est une *première étape*
pour la création de la liste de vocabulaire associée au texte.
En effet, il propose toutes les possibilités (connues)
de lemmatisation pour les formes du texte.
Ainsi, pour obtenir le résultat final (c'est-à-dire le lemme unique
qui correspond à la forme dans ce contexte),
l'helléniste doit lire tout le fichier et supprimer
(ou parfois corriger) les lignes inutiles.

Le fichier CSV propose un lemme par ligne.
Si une forme du texte peut être associée à plusieurs lemmes,
il apparaîtra dans autant de lignes que nécessaire.
Une ligne est formée de 6 champs, séparés par une *tabulation*
(certains l'appelleront un TSV, mais cela reste un
format de valeurs séparées par un caractère). Les champs sont :

* un nombre qui reflète le rang du mot dans le texte

* la forme telle qu'elle est dans le texte

* le lemme associé en caractères grecs

* une courte traduction

* le lemme en betacode

* le lemme en betacode sans les signes diacritiques.

### TextiColor et correction OCR

Le TextiColor a été introduit pour la première fois dans Collatinus
à des fins pédagogiques (demandé par Jan Bart, un enseignant néerlandais).
Il a également été réalisé que l'ajout de couleurs au texte
peut simplifier la recherche
de fautes de frappe et d'autres erreurs liées à l'OCR.

Lorsque l'option TextiColor est cochée, l'outil _txt2csv_
produira, avec le fichier CSV,
un deuxième fichier (avec l'extension **htm**) qui
contient le texte original, mais avec quelques mots mis en évidence.
Ces mots sont écrits en **gras**, et ils
correspondent à des formes qui n'ont pas été bien reconnues
par le lemmatiseur.
La _gravité_ du problème est codée dans la couleur.

* **rouge** : la forme est inconnue

* **bleu** : il y a des différences dans les signes diacritiques

* **noir** : la majuscule était inattendue.

Le fichier **htm** (qui peut être facilement converti 
en un fichier **odt** pour l'édition)
donne ainsi quelques indications pour corriger le texte.
Les mots qui apparaissent en **gras** doivent être vérifiés
(mais ils pourraient être corrects).
Pour les mots **bleus**, des suggestions peuvent être trouvées
dans la liste de vocabulaire. En cas de désaccord sur les signes diacritiques,
la forme approximative (trouvée dans la base de données) est donnée entre
parenthèse après le lemme. Par exemple, si le texte contient
_οὐδε_, la lemmatisation donnera (entre autres solutions)
_οὐδός2 (οὐδέ)_.

**NB** : si l'erreur OCR conduit à un mot existant,
le TextiColor sera incapable de le révéler.

## Histoire

Ce projet est la version hors ligne d'Eulexis sur
[Biblissima](http://outils.biblissima.fr/fr/eulexis/).
Il est écrit en Qt.

Initialement, il était prévu pour ouvrir des dictionnaires grecs
comme [Collatinus](http://outils.biblissima.fr/fr/collatinus/) le fait.
Ensuite, il a été étendu avec un lemmatiseur de forme,
qui utilise, avec l'accord de l'auteur,
les analyses grecques de [Diogenes](https://d.iogen.es/d/).
J'ai changé le format du fichier pour permettre
la traduction dans d'autres langues, actuellement
français et allemand. Notez que ces traductions
ont été obtenus avec Google Trad et doivent maintenant être
corrigées.
