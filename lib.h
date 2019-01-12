#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED
#include <stdio.h>

#define BUFFSIZE 1024
#define NUM_BUILTINS 10
#define SEP " \t\r\n\a"

typedef struct alias_
{
    char* name;
    char* value;
    struct alias_* next; 
} alias;

typedef struct envrn_
{
    char* name;
    char* value;
    struct envrn_* next; 
} envrn;

typedef struct variable
{
    char* name;
    char* value;
    struct variable* next; 
} variable;

// les commandes internes de mpsh

int export(char** args);                                                        //  export une variable
int type(char **args);                                                          //  retourne le type d'une commande
int history(char** args);                                                       //  affiche l'historique des commandes
int cmd_alias(char** cmd);                                                      //  affiche/met en place un alias
int cmd_unalias(char** cmd);                                                    //  supprime un alias
int cmd_exit(char** cmd);                                                       //  exit le programme
int cmd_pwd(char** cmd);                                                        //  affiche le repertoire courant
int mpsh_echo (char** args);                                                    //  affiche la valeur de la variable
int mpsh_cd(char **args);                                                       //  change de repertoire
int mpsh_umask(char **args);                                                    //  change la valeur d'umask

// les fonctions utilisées par les commandes internes

int malloc_env(char* var, char* val);                                           // aloue de l'espace pour une variable d'environement
envrn* recherche_env(char* name);                                               // recherche une variable d'environement
int set_env(char* name,char *value);                                            // affecte une valeur a une variable d'environement
void print_env();                                                               // affiche les variables d'environements existantes
char** split(char* str, char delim);                                            // divise un string en deux selon un separateur

int check_affect(char** args);                                                  // verifie si une commande est une affectation simple

int count_lines (FILE* fp);                                                     // compte le nombre de ligne d'un fichier
int string_num(char *s,int base, int *result );                                 // converte un string vers un chiffre

void affiche_alias();                                                           // affiches les alias existants
int recherche_alias(char name[],alias** elem,alias** preced);                   // recherche un alias
void ajout_alias(alias *elem);                                                  // ajoute un alias
int valid_name(char *name);                                                     // verifie si le nom d'un alias est valide

void sub_transf(char *token, char *path);                                       // fonction auxilaire de transform_chemin
char* transform_chemin(char* chemin);                                           // transforme un chemin de mpsh vers un chemin de bash

int malloc_var(char* var, char* val);                                           // aloue de l'espace pour une variable simple
variable* recherche_var(char* var);                                             // recherche une variable 


#endif
