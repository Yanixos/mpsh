PROJET C & SYSTÃˆME
=====

**L3 Informatique**

Il est important de bien lire le sujet jusqu'au bout et de bien y rÃ©flÃ©chir avant de se lancer dans la programmation du projet.

## Sujet : `mpsh`, _aka_ mon petit shell

Le but de ce projet est de vous faire Ã©crire un interprÃ©teur de commandes (connu Ã©galement sous le nom de _shell_). Dans le reste de l'Ã©noncÃ©, nous appellerons ce shell `mpsh`.

Le shell `mpsh` fonctionnera comme les shells les plus courants : une invite de commande attendra la demande de l'utilisateur, puis y rÃ©pondra. Cette demande prendra la forme d'une combinaison de commandes simples et d'Ã©ventuelles redirections, une commande simple Ã©tant soit une affectation de variable, soit une commande (interne Ã  `mpsh` ou externe), soit un alias, suivi d'Ã©ventuelles options et d'Ã©ventuels arguments. Les combinaisons possibles de commandes et les redirections sont dÃ©taillÃ©es dans la partie _EnchaÃ®nements de commandes et redirections_ ci-dessous.

`mpsh` est un shell assez Ã©lÃ©mentaire, mais un utilisateur habituel d'un shell usuel comme `bash` doit pouvoir s'en servir sans difficultÃ© ; on vous demande en particulier de respecter les points suivants :

* le shell `mpsh` doit supporter les rÃ©fÃ©rences de fichiers relatives et absolues ;
* les commandes sur `mpsh` sont toutes lancÃ©es en avant-plan ;
* les commandes externes qui fonctionnent sous les shells usuels doivent continuer Ã  fonctionner sous `mpsh` (et on ne vous demande Ã©videmment pas de les rÃ©Ã©crire).

Ã€ son lancement, `mpsh` devra lire, s'il existe, le fichier de configuration `~/.mpshrc`. La description des informations contenues dans ce fichier apparaÃ®t rÃ©partie Ã  plusieurs endroits dans la suite du sujet.

Pour chaque fonctionnalitÃ© demandÃ©e ci-dessous, merci de coller au plus prÃ¨s avec les notations de `bash`, cela permettra des tests plus rapides de vos programmes.  

#### FonctionnalitÃ©s

Nous dÃ©crivons ici les fonctionnalitÃ©s de votre shell. Certaines seront implÃ©mentÃ©es comme commandes internes, d'autres comme commandes externes, Ã  vous de dÃ©cider. Pour vous faire une idÃ©e, vous pouvez consulter la liste des commandes internes de `bash` Ã  la page du manuel de `bash-builtins`, et celle de `zsh` Ã  la page du manuel de `zshbuiltins`. Vous avez le droit d'utiliser des commandes externes qui existent dÃ©jÃ  sur les ordinateurs des salles de TP de l'UFR si cela fonctionne. 

Les fonctionnalitÃ©s qu'on doit pouvoir utiliser avec `mpsh` sont les suivantes (les arguments optionnels pour une commande sont spÃ©cifiÃ©s entre crochets [ ]; quand l'action d'une option n'est pas prÃ©cisÃ©e, c'est qu'elle correspond Ã  la mÃªme option que la commande courante) :

* `alias [name=value]` : affiche les alias ou met en place un alias
* `cat ref` : affiche le contenu de la rÃ©fÃ©rence
* `cd [dir]` : change le rÃ©pertoire courant
* `echo $var` : affiche la valeur de la variable `var`
* `exit [n]` : permet de sortir du shell avec la valeur de retour `n` si `n` est spÃ©cifiÃ©, la valeur de retour de la derniÃ¨re commande lancÃ©e sinon
* `export var[=word]` :  exporte une variable ( _i.e._ la transforme en variable d'environnement)
* `history [n]` :
	* sans argument, affiche la liste numÃ©rotÃ©e de l'historique des commandes,
	* avec un argument `n` entier positif, relance la commande dont le numÃ©ro dans la liste est `n`,
	* avec un argument `-n` entier nÃ©gatif, fixe Ã  `n` le nombre de commandes enregistrÃ©es dans l'historique.
* `ls [ref]` : liste le contenu d'un rÃ©pertoire (le rÃ©pertoire courant en l'absence d'arguments)
* `mkdir [-p] ref` : crÃ©e un nouveau rÃ©pertoire
* `pwd` : affiche la rÃ©fÃ©rence absolue du rÃ©pertoire courant
* `type name [name ...]` : indique comment chaque nom est interprÃ©tÃ© (comme alias, commande interne ou commande externe) s'il est utilisÃ© pour lancer une commande
* `umask mode` : met en place un masque pour les droits
* `unalias name` : supprime un alias

#### Variables

`mpsh` doit gÃ©rer les variables : on doit pouvoir en dÃ©finir, afficher les variables existantes et spÃ©cifier qu'une variable n'existe plus ; de plus l'export de variables doit Ãªtre supportÃ©.

La gestion des variables suivantes vous est imposÃ©e :

* `?` donne la valeur de retour de la derniÃ¨re commande lancÃ©e.

* `INVITE` contient le format du texte affichÃ© par l'invite de commande de `mpsh` ; il y a une invite par dÃ©faut, mais l'utilisateur doit pouvoir dÃ©finir le format de l'invite dans son fichier de configuration `~/.mpshrc` (Ã©quivalent de la variable `PS1` en `bash`).

* `CHEMIN` est une variable d'environnement qui joue le rÃ´le de la variable `PATH` en `bash` : `CHEMIN` est une suite de rÃ©fÃ©rences de rÃ©pertoires espacÃ©es par deux points (` :`) ; lorsque l'utilisateur lance une commande qui n'est ni un alias, ni une commande interne, l'exÃ©cutable doit Ãªtre cherchÃ© dans les rÃ©pertoires listÃ©s par `CHEMIN` (l'alias, puis la commande interne, s'il en existe du nom donnÃ©, sont toujours prioritaires dans ce choix). Quand le nom d'un rÃ©pertoire listÃ© dans `CHEMIN` est suivi de `//`, alors la recherche de l'exÃ©cutable est faite dans toute la sous-arborescence. C'est le premier exÃ©cutable trouvÃ© par la recherche d'exÃ©cutable qui doit Ãªtre exÃ©cutÃ© (dans le cas de sous-arborescences, l'ordre de visite des fichiers n'est pas imposÃ©).  
Par exemple, si on dÃ©finit `CHEMIN=~/bin//:/bin`, le lancement de la commande `ls` doit amener `mpsh` Ã  vÃ©rifier dans l'ordre (en s'arrÃªtant dÃ¨s que c'est possible) : s'il y a un alias `ls`, s'il y a une commande interne `ls`, s'il y a une commande `ls` dans l'arborescence de racine `~/bin`, s'il y a une commande `/bin/ls`. Si aucune de ces recherches n'aboutit, `mpsh` doit afficher un message d'erreur.


#### EnchaÃ®nements de commandes et redirections

On veut pouvoir enchaÃ®ner des commandes avec les connecteurs logiques *"et"* (`&&`) et *"ou"* (`||`). L'Ã©valuation doit se faire de faÃ§on paresseuse, c'est-Ã -dire que l'exÃ©cution de la deuxiÃ¨me commande est conditionnÃ©e au rÃ©sultat de l'exÃ©cution de la premiÃ¨re.
Les enchaÃ®nements d'un nombre quelconque de commandes doivent Ãªtre possibles. On supposera pour simplifier que le parenthÃ©sage se fait implicitement de droite Ã  gauche.

`mpsh` doit gÃ©rer les redirections d'entrÃ©e (`<`), de sortie (`>`) et de sortie erreur (`2>`), et permettre l'enchaÃ®nement de commandes par tubes (`|`).

Il n'est pas demandÃ© de pouvoir combiner sur une mÃªme ligne de commande des connecteurs logiques et des redirections.

#### Conventions syntaxiques

Pour simplifier la lecture d'une ligne de commande, on suppose que les caractÃ¨res utilisÃ©s dans une ligne de commande sont soit des espaces, soit de type alphanumÃ©rique, soit un des caractÃ¨res suivants :

`.` `_` `$` `/` `~` `-` `=` `?` `:` `>` `<` `|` `&`

L'espace servira uniquement et systÃ©matiquement Ã  sÃ©parer deux Ã©lÃ©ments diffÃ©rents de la ligne de commande (attention : une affectation de variable est considÃ©rÃ©e comme un unique Ã©lÃ©ment, donc pas d'espaces autour du signe `=`). On ne vous demande pas de gÃ©rer un caractÃ¨re d'Ã©chappement (en consÃ©quence on suppose qu'aucun fichier ne contient d'espace dans son nom par exemple). Le caractÃ¨re `$` correspond toujours Ã  l'Ã©valuation de la variable dont le nom suit ce caractÃ¨re.

#### ComplÃ©tions automatiques

`mpsh` doit supporter la complÃ©tion automatique par tabulation : la frappe du caractÃ¨re `TAB` aprÃ¨s un dÃ©but de chaÃ®ne de caractÃ¨res (Ã©ventuellement vide) doit complÃ©ter la ligne courante de maniÃ¨re vraisemblable :

* si l'utilisateur est en train d'Ã©crire une commande, une suite vraisemblable doit mener au plus long prÃ©fixe possible de nom de commande (s'il en existe),
* si l'utilisateur est en train d'Ã©crire une rÃ©fÃ©rence de fichier, une suite vraisemblable doit mener au plus long prÃ©fixe possible de rÃ©fÃ©rence de fichier (au sens large et s'il en existe).

En particulier, l'utilisateur doit pouvoir dÃ©finir dans son fichier de configuration `~/.mpshrc` quelles extensions sont liÃ©es Ã  une commande particuliÃ¨re. On peut imaginer par exemple, et sans que ce format ne soit imposÃ©, que la prÃ©sence de la ligne

`complete gcc c`

dans le fichier `~/.mpshrc` signifie que la complÃ©tion automatique Ã  la suite de la commande `gcc` ne propose que les rÃ©fÃ©rences de fichiers ayant `.c` pour suffixe et les Ã©ventuels sous-rÃ©pertoires du rÃ©pertoire courant.

Pour implÃ©menter les complÃ©tions automatiques, et de faÃ§on gÃ©nÃ©rale pour gÃ©rer l'Ã©dition de la ligne de commande, nous vous recommandons vivement d'utiliser la librairie `readline` fournie par `GNU`. Cette librairie est documentÃ©e sur internet, par exemple sur le [site du MIT](http://web.mit.edu/gnu/doc/html/rlman_toc.html). Voici un exemple minimal d'utilisation de la librairie :

```
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
  while(1){
    char *s = readline("mon prompt>>");
    traitement_ligne_de_commande(s);
    free(s);
  }
  return 0;
}
```

Un exemple plus complexe qui met en place une complÃ©tion automatique diffÃ©rente de celle qui vous est demandÃ©e : [ex_completion.c](../blob/master/Projet/ex_completion.c).

La compilation d'un programme utilisant la librairie `readline` se fait en explicitant le lien Ã  cette librairie grÃ¢ce Ã  l'option `-lreadline` de `gcc` (dans un `Makefile`, vous pouvez passer par la dÃ©finition de `LDLIBS = -lreadline`).

## ModalitÃ©s de rendu

Le projet est Ã  faire par Ã©quipes de 2 ou 3 Ã©tudiants. Aucune exception ne sera tolÃ©rÃ©e. La composition de chaque Ã©quipe devra Ãªtre envoyÃ©e par mail aux enseignants de cours d'amphi de C et de systÃ¨mes au plus tard le 2 novembre 2018, avec copie Ã  chaque membre de l'Ã©quipe.

Chaque Ã©quipe doit crÃ©er un dÃ©pÃ´t `git` privÃ© sur le [gitlab de l'UFR](http://moule.informatique.univ-paris-diderot.fr:8080/) **dÃ¨s le dÃ©but de la phase de codage** et y donner accÃ¨s en tant que `Reporter` Ã  tous les enseignants des cours de C et SystÃ¨me : Wieslaw Zielonka, Roberto Amadio, Benjamin Bergougnoux, Constantin Enea et Alexandre Nolin pour C ; Ines Klimann, Pierre Letouzey et Dominique Poulalhon pour SystÃ¨me. Le dÃ©pÃ´t devra contenir un fichier `equipe` donnant la liste des membres de l'Ã©quipe (nom, prÃ©nom, numÃ©ro Ã©tudiant et pseudo(s) sur le gitlab).

**Nous vous fournirons un programme pour tester des fonctionnalitÃ©s Ã©lÃ©mentaires de votre projet** (essentiellement le contenu des parties _FonctionnalitÃ©s_ et _EnchaÃ®nements de commandes et redirections_). **Si votre projet ne peut rÃ©aliser ce minimum, il sera considÃ©rÃ© comme vide et entraÃ®nera la note 0.** Attention : c'est un minimum pour une note positive, pas pour une note au-dessus de la moyenne.

Bien traiter ce qui est explicitement demandÃ© dans cet Ã©noncÃ© suffira pour obtenir une trÃ¨s bonne note, il n'est pas utile d'en faire plus Ã  cette fin. Ce n'est pas interdit non plus, mais il est important de vous assurer que le minimum fonctionne et est programmÃ© dans les rÃ¨gles de l'art.

Votre shell `mpsh` doit fonctionner sur les ordinateurs des salles de TP de l'UFR (2031 et 2032). Le programme doit compiler avec les options `-Wall` (sans avertissement) et `-g` de `gcc` et doit pouvoir Ãªtre lancÃ© quel que soit le rÃ©pertoire courant, avec la commande `mpsh`, sous votre shell habituel et sous `mpsh`.

En plus du programme demandÃ©, vous devez fournir un `Makefile` utilisable et des descriptions de toutes les commandes que vous avez implÃ©mentÃ©es, sous une mise en page semblable Ã  une page de manuel (mais on ne vous demande pas que ce soit consultable grÃ¢ce Ã  la commande `man`).

En cas de question et si la rÃ©ponse n'est pas contenue dans le prÃ©sent document, merci de poser la question sur le forum `moodle` dÃ©diÃ© du cours de systÃ¨mes. Seules les rÃ©ponses postÃ©es sur ce forum feront foi au moment de la soutenance.

Les seules interdictions strictes sont les suivantes : plagiat (d'un autre projet ou d'une source extÃ©rieure Ã  la licence), utilisation de la fonction `system` de la `stdlib`.

## Soutenances

La soutenance se fera Ã  partir du code contenu sur le gitlab. Elle aura vraisemblablement lieu pendant la semaine de rentrÃ©e de janvier, la date prÃ©cise sera ultÃ©rieurement affichÃ©e sur `moodle`.
