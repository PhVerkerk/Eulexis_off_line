# Eulexis_off_line

Eulexis is an **Ancient Greek lemmatisation tool**. 

It is a free and open source software available for Mac OS and Windows. 
It is also compatible with GNU/Linux, so you can compile this software from the source code available in this repository (see [this comment](https://github.com/PhVerkerk/Eulexis_off_line/issues/3#issue-737616080)).

Installation packages for Mac OS and Windows are downloadable on this page: https://outils.biblissima.fr/en/eulexis/

### Update 2020-01-18

## Vocab-list and OCR correction
With a sollicitation coming from David Carter,
I have introduced the possibility to generate
a vocab-list in CSV from any Greek text.
It is just a new item in the File menu, called _txt2csv_.
It opens a dialog window to open a text file 
(with .txt extension)
and will produce a CSV file with the same name
(but with the .csv extension). 
If the destination file already exists,
it will be overwritten without warning.

*NB* : it is not necessary to open the text in Eulexis'
text-window and to lemmatize it.

This _txt2csv_ tool has two extra options :

       - TextiColor

       - BOM4ms, this option adds a Byte Order Marker 
       (Unicode pseudo-character) at the beginning of the CSV-file
       to facilitate some microsoft products to understand
       that the character encoding is UTF-8.

### Vocab-list
The CSV-file produced by _txt2csv_ is a *first step*
for the creation of the vocab-list associated to the text.
As a matter of fact, it proposes all the possible (known)
lemmatizations for the forms of the text.
So, to obtain the final result (i.e. the unique lemma
that corresponds to the form in this context), 
the Hellenist has to read all the file and to suppress
(or sometimes correct) the un-needed lines.

The CSV-file proposes one lemma per line.
If a form of the text can be associated to several lemmata,
it will appear in as many lines as needed.
A line is formed of 6 fields, separated by a *tab*
(some will call it a TSV, but it remains a Character
Separated Values format). The fields are :

      - a number that reflects the rank of the word in the text

      - the form as it is in the text

      - the associated lemma in Greek characters

      - a short translation

      - the lemma in betacode

      - the lemma in betacode without the diacritics.

### TextiColor and OCR correction
The TextiColor has been first introduced in Collatinus
for teaching purposes (asked by Jan Bart, a Dutch lecturer).
It has been also realized that adding colors to the text
according to some conventions can simplify the research
of typos and other OCR related mistakes.

When the TextiColor option is checked, the _txt2csv_ tool
will produce, together with the CSV-file, 
a second file (with .htm extension) that 
contains the original text, but with some words highlighted.
These words are written in *bold*, and they
correspond to forms that have not been fully recognized
by the lemmatizer. 
The _gravity_ of the problem is encoded in the color.

       - *red* : the form is unknown

       - *blue* : there are some discrepancies in the diacritics

       - *black* : the uppercase was unexpected.

The .htm file (which can be easily converted to an .odt file for editing)
then gives some hints to correct the text.
The words that appear in *bold* have to be checked
(but they might be OK).
For the *blue* words, some suggestions can be found
in the vocab-list. In case of discrepancies in the diacritics,
the approximate form (found in the data-base) is given between 
parenthesis after the lemma. For instance, if the text contains
_οὐδε_, the lemmatization will give (among other solutions) 
_οὐδός2 (οὐδέ)_.

*NB* : if the OCR mistake leads to an existing word,
the TextiColor tool will be unable to reveal it.

## History
This project is the off-line version of Eulexis on 
[Biblissima](http://outils.biblissima.fr/fr/eulexis/).
It is written in Qt.

Initially, it was intended to open Greek dictionaries
in the way [Collatinus](http://outils.biblissima.fr/fr/collatinus/) does it.
Then it has been extended with a form lemmatizer,
that uses, with the agreement of the author,
the greek-analyses of [Diogenes](https://community.dur.ac.uk/p.j.heslin/Software/Diogenes/).
I have changed the format of the file to allow
the translation to other languages, presently
French and German. Please note that these translations
have been obtained with Google Trad and need now to be
corrected.
