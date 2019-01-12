Usage :
            Dans le repertoire qui contient le make, lancer la commande : make
            lancer la commande : ./mpsh
            Pour nettoyer : make clean

Les fichiers :

            lib.h  : la bibliotheque qui contient la declaration des structures et les prototypes des commandes internes
            lib.c  : le corp des commandes internes et des fonctions auxiliaires utilisés
            mpsh.c : contient le main et les fonctions utilisés par mpsh
            
Fonctionalités :

Affichage:
            Une fois le code exécuté, l'invite du shell s'afficherait sous le format suivant: <nomutilisateur@mpsh: rép_curr>

Traitement de l'entrée:

            Obtenir ligne par ligne en utilisant, getline ()
            Traitement de chaque commande à l'aide de strtok (), avec certains DELIMITERS, et identification de la commande.
            Vérification également si deux commandes sont séparées par '&&' ou '||'. 
            A cette étape, la commande et les arguments, les options sont séparés, pour chaque commande identifiée.
            Tout d’abord, vérifiez s’il s’agit d’un shell intégré ou non.
            Sinon, exécutez la commande avec execvc, sinon envoyez-la à la fonction appropriée.

Manuel des commandes internes :

cat ref : Permet d'afficher le contenu de la référence (ref) dans la sortie standard(stdout)

umask mode : Permet de définir les permissions par défaut d'un répertoire ou d'un fichier créé.

cd [dir] : Sans arguments :  ramène au répertoire par défaut de l'utilisateur.
           Avec arguments :  permet de changer de répertoire courant (dir nouveau répertoire s'il existe sinon on affiche une erreur
           					 dans la sortie d'erreur standard (stderr)

echo $var : Permet d'afficher la valeur de la variable var si elle existe sinon une erreur est renvoyée dans la sortie d'erreur standard(stderr)
             2-echo alphanum : Affiche la chaine alphanum dans la sortie standard(stdout)


history : Sans argument : permet d'afficher la liste numérotée de l'historique des commandes,
		  Avec un argument n entier positif  : relance la commande dont le numéro dans la liste est n,
          Avec un argument -n entier négatif : fixe à n le nombre de commandes enregistrées dans l'historique.

mkdir [-p] ref : crée un répertoire correspondant au nom mentionné.
                 -p : Cette option permet de créer un ensemble de dossier plutôt qu'un dossier seul. Elle ne retourne pas d'erreur si le dossier à créer existe déjà:


type name [name] : indique comment chaque nom est interprété (comme alias, commande interne ou commande externe) s'il est utilisé pour lancer 	 
				   une commande.


Piping : Permet de passer le résultat d'une commande shell à une autre commande shell . Les commandes sont séparées en utilisant le symbole '|'

Redirections : - Les pointeurs de fichiers sont remplacés par STDIN/STDOUT en utilisant dup2.
               - “<” :  l'entrée est lue à partie du fichier donné .
               - ">" : les fichiers sont ouverts pour la concaténation/écrasement et pour la création d'un fichier s'il n'existe pas. 
               - Erreurs gérées : Erreur/Absence  de fichiers d'entrée, de sortie.




alias: alias [nom[=valeur] ... ]
	*alias sans arguments affiche la list des aliases sous la forme 
	NOM=VALEUR dans la sortie standard (stdout).
	*alias NOM=VALEUR .. définie un alias pour chaque NOM en lui attribuant VALEUR
	*alias NOM affiche s'il existe dans la list des aliases, la valeur de l'alias NOM, 	sinon elle affiche une erreur dans la sortie d'erreur standard (stderr) 
	*alias vérifie que le NOM est valid.
	*alias accepte un enchaînement d'affectation et d'affichage de plusieurs alias 
	á la fois  

unalias: unalias nom [nom ...]
	*unalias supprime l'alias NOM de la liste des alias s'il existe
	*unalias affiche une erreur dans la sortie d'erreur standard (stderr)
	s'il l'alias NOM n'existe pas dans la list
	*unalias accepte un enchaînement de plusieurs NOM d'alias á la fois 

exit: exit [n]
	*exit avec l'argument n, quitte le shell avec comme valeur de retour n.
	*exit sans l'argument, quitte le shell avec la valeur de retour de la dernière 
	
pwd: pwd
	*affiche le répertoire courant dans la sortie standard(stdout) 

export:export var[=mot]
	*export sans arguments affiche la list de tous les noms exporté dans ce shell dans 	la sortie standard (stdout).
	*export VAR=MOT définie une variable d'environnement VAR en lui attribuant MOT.
	*export VAR affiche s'il existe dans la list des variables d'environnement dans ce  	shell, la valeur de la variable NOM, sinon elle affiche une erreur dans la sortie 	d'erreur standard (stderr) 
	*export vérifie que le NOM est valid.

ls: ls [ref] 
	*ls sans l'argument ref affiche le contenu du repertoire courant
	*ls affiche le contenu du repertoire dont le chemin est ref si le chemin est 		correct sinon elle affiche une erreur sur la sortie des erreurs standard.  
