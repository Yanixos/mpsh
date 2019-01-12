#define  _POSIX_C_SOURCE  200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "lib.h"

char* mpsh_prompt();                                                            // genere le prompt par defaut
void mpsh_loop();                                                               // la boucle generale qui lis les commandes
int add_cmd_mpsh_history(char* cmd);                                            // ajoute les commandes entrées à l'historique
int mpsh_launch(char **args);                                                   // lance les commandes externes 
char** make_args(char* cmd);                                                    // transforme un string vers un pointeur de strings
char *replace_str(char *str, char* orig,char* p);                               // remplace $HOME par ~ ( utilisé par le prompt )
int isLastCommand(char *cmd,FILE* fp);                                          // vérifie si la commande entrée est la meme que la derniere
int check_operations(char** args,char* temp);                                   // vérifie s'il existe des operations && ||
int find_pipe_redirections(char* cmd,int* out_err);                             // recherche l'indice d'un pipe ou d'une redirection
int execute_pipe(char** args1, char** args2);                                   // execute une commande qui contient un pipe
int execute_redirections(char** args1, char** args2, char* temp, int pos);      // execute une commande qui contient une redirection
int parse_pipe_redirections(char *temp,int pos,int err);                        // parse une ligne de commande pour trouver un pipe ou red
int mpsh_execute(char** args);                                                  // execute les commandes entrées par l'utilisateur
char** split_command(char *line);                                               // divise une commande en plusieurs tokens
char* mpsh_path(char** argv);                                                   // trouve le chemin de l'executable de mpsh
int create_mpshrc(char* mpshrc, char** argv);                                   // crée le fichier de configuration dans $HOME/.mpshrc
int load_mpshrc(char** args);                                                   // load le fichier de configuration dans $HOME/.mpshrc


int initialize_readline();                                                      // initialise du completeur automatique
char *command_generator(const char *, int);                                     // genere les commandes possibles comme completion actuelle
char **fileman_completion(const char *, int, int);                              // genere les fichiers possibles comme completion actuelle
int filter_filename_completion(char **matches);                                 // filtre les fichiers selon la commande entrée

extern alias* als;
extern envrn* env;
extern variable* vars;

extern char *builtins_name[];
extern int (*builtins_func[]) (char **);

int main(int argc, char** argv)
{
    load_mpshrc(argv);
    mpsh_loop();
    
    return 0;
}


void mpsh_loop()
{

    char exit_val[32];
    char* prompt;
    variable* invite;
    char *cmd;
    char** args;
    int status;
    
    status = malloc_var("?","0");                                               // initialiser la valeur du retour "?" a 0
    initialize_readline ();                                                     // initialiser la completion automatique

    do
    {
        // si invite à une valeur differente du celle par defaut, alors on l'utilise comme prompt
        if ( ( invite = recherche_var("INVITE") ) != NULL && strstr(invite->value,"@mpsh") == NULL )  
            prompt = invite->value;
        // sinon, on utilise le prompt par defaut
        else
            prompt = mpsh_prompt (); 
            
        cmd = readline (prompt);                                                // lecture du commande
        
        if ( ! cmd  )                                                           // si il s'agit d'un CNTRL+D
            break;                                                             // on sort de la boucle

        status = add_cmd_mpsh_history(cmd);                                     // ajout de la commande à l'historique
        args = split_command (cmd);                                             // divise la commande en tokens
        status = mpsh_execute (args);                                           // execute la commande
        sprintf(exit_val,"%d",status);                                          
        variable* e = recherche_var("?");
        e->value = strdup(exit_val);                                            // mis a jour la valeu de la variable '?' exit status

        free(cmd);                                                              
        free(args);
        
    } while (1);

    free(cmd);
    fprintf(stdout,"\n");

    exit(1);
}

char* mpsh_prompt()
{
    char *orig = strdup(getenv("HOME"));
    char prompt[BUFFSIZE];
    char cwd[255];
    char *p;

    getcwd(cwd, sizeof(cwd));
    sprintf(prompt,"%s%s%s%s",strdup(getenv("USER")),"@mpsh:",cwd,"$ ");
    
    if ( ( p = strstr(prompt, orig) ) != NULL )
        return replace_str(prompt,orig,p);
    else
        return strdup(prompt);
}

char *replace_str(char *str,char* orig,char* p)
{
    char buffer[BUFFSIZE];
    char* rep = strdup("~");

    strncpy(buffer, str, p-str); 
    buffer[p-str] = '\0';

    sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
    return strdup(buffer);
}

int isLastCommand(char *cmd,FILE* fp)
{
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int i=1;    
    int nb_lines = count_lines(fp);
    
    rewind(fp);
    strcat(cmd,"\n");    
    
    while ( ( read = getline(&line, &len, fp) ) != -1 )
    {
        if ( i++ == nb_lines )
            return strcmp(line,cmd);  
    } 
    return -1;   
}

int add_cmd_mpsh_history(char *cmd)
{

    char* mpsh_history = strdup(getenv("HOME"));
    strcat(mpsh_history,"/.mpsh_history");
    FILE * fp;
    
    if ( ( fp = fopen(mpsh_history,"a+") )== NULL)                              // lecture du fichier historique des commandes mpsh
        return -1;
    if ( isLastCommand(cmd,fp) )                                                // si la commande actuelle est differente de la derniere
        if ( ( fprintf(fp,"%s",cmd) ) == -1 )                                   // on l'ajoute
            return -1;
    
    fclose(fp);
    return 0;   
}

int mpsh_launch(char **args)                                                    
{

    char* ch = recherche_env("CHEMIN")->value;                                  // recuperer le chemin mpsh
    char* chemin = transform_chemin(ch);                                        // regulareser le chemin pour qu'il soit compatible avec le bash
    char* old_path = strdup(getenv("PATH"));                                    // sauvegarde du path bash
    
    setenv("PATH",chemin,1);                                                    // mettre a jour le chemin bash

    pid_t pid;
    int status;

    pid = fork();
    
    if (pid == -1) 
    { 
        fprintf(stderr,"mpsh: echec dans fork()."); 
        return -1; 
    } 
    else if (pid == 0)                                                         // le processus fils 
    { 
        if ( execvp(args[0], args) < 0) 
        {
            fprintf(stderr,"mpsh: %s: commande introuvable.\n",args[0]);        // executer la commande externe
            setenv("PATH",old_path,1);                                          // restaurer le path bash
            set_env("CHEMIN",chemin);                                           // restaurer le path mpsh
            return -1;
        }
        exit(-1);
    } 
    else                                                                        // le processus pere
    { 
         do 
         {
                setenv("PATH",old_path,1);                                      // restaurer le path bash
                set_env("CHEMIN",chemin);                                       // restaurer le path mpsh
                waitpid(pid, &status, WUNTRACED);
         } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    } 
   
    return 0;
}

int check_operations(char** args,char* temp)
{    
    char* s;
    if ( ( s = strstr(temp," && ") ) != NULL )                                  // si on trouve && 
    {
        int pos = s - temp;
        if ( pos == 0 || pos == strlen(temp) - 4  )                             // alors si il est au debut ou a la fin, donc erreur
        {
            fprintf(stderr,"mpsh: %s: commande introuvable.\n",temp);
            return -1;
        }
        else                                                                    // sinon on divise en deux commandes
        {
            int j=0;
            int i=0;
            char* cmd1 = (char*) calloc(BUFFSIZE,sizeof(char) );
            char* cmd2 = (char*) calloc(BUFFSIZE,sizeof(char) );
            
            for (i=0; i < pos; i++)
                cmd1[i] = temp[i];
            cmd1[i] = '\0';
            
            for (int i = pos+4; i < strlen(temp); i++)
                cmd2[j++] = temp[i];
            cmd2[j] = '\0';

            int ret1 = mpsh_execute(split_command(cmd1));                       // si la premiere commande est bien executé 
            if ( ! ret1 )
                return mpsh_execute(split_command(cmd2));                       // on execute la deuxieme
            else 
                return -1;
        }
    }
    
    else if ( ( s = strstr(temp," || ") ) != NULL )                            // si on trouve || 
    {
        int pos = s - temp;
        if ( pos == 0 || pos == strlen(temp) - 4 )                              // alors si il est au debut ou a la fin, donc erreur
        {
            fprintf(stderr,"mpsh: %s: commande introuvable.\n",temp);
            return -1;
        }
        else                                                                    // sinon, on divise en deux commandes
        {
            int j=0;
            int i=0;
            char* cmd1 = (char*) calloc(BUFFSIZE,sizeof(char) );
            char* cmd2 = (char*) calloc(BUFFSIZE,sizeof(char) );
            
            for (i=0; i < pos; i++)
                cmd1[i] = temp[i];
            cmd1[i] = '\0';
            
            for (int i = pos+4; i < strlen(temp); i++)
                cmd2[j++] = temp[i];
            cmd2[j] = '\0';
            
            
            int ret1 = mpsh_execute(split_command(cmd1));                       // si la premiere est bien executé
            if ( ! ret1 )       
                return 0;                                                      // alors succes
            else 
                return mpsh_execute(split_command(cmd2));                      // sinon, on execute la deuxieme
        }
    }
    
    else
        return 1;
}
        
int find_pipe_redirections(char* cmd,int* out_err)                              
{
    int len = strlen(cmd);
    char* pipe; int pos_pipe=BUFFSIZE;
    char* out;  int pos_out=BUFFSIZE;
    char* in;   int pos_in=BUFFSIZE;
    char* err;  int pos_err=BUFFSIZE;
    
    if ( ( pipe = strstr(cmd,"|") ) != NULL )
    {
        pos_pipe = pipe - cmd;
        if ( pos_pipe == 0 || pos_pipe == len - 1 )                             // chercher la position du |
            pos_pipe=BUFFSIZE;
    }
    
    if ( ( out = strstr(cmd,">") ) != NULL )                                   
    {
        pos_out = out - cmd;
        if ( pos_out == 0 || pos_out == len - 1 )                               // chercher la position du >
            pos_out=BUFFSIZE;
    }
    
    if ( ( in = strstr(cmd,"<") ) != NULL )
    {
        pos_in = in - cmd;
        if ( pos_in == 0 || pos_in == len - 1 )                                 // chercher la position du <
            pos_in=BUFFSIZE;
    }
    
    if ( ( err = strstr(cmd," 2>") ) != NULL )
    {
        pos_err = err - cmd;
        if ( pos_err == 0 || pos_err == len - 3 )                               // chercher la position du 2>
            pos_err=BUFFSIZE;
        else
            *out_err = 1;
    }
    
    if ( pos_pipe == pos_out && pos_out == pos_in && pos_in == pos_err && pos_err == BUFFSIZE )
        return -1;
    
    int t1 = pos_pipe <= pos_out ? pos_pipe : pos_out;
    int t2 = pos_in   <= pos_err ? pos_in   : pos_err;
    
    return ( t1 <= t2 ? t1 : t2 );                                             // retourner la position minimum entre les 4
}

int execute_pipe(char** args1, char** args2)
{
    int pipefd[2];  
    pid_t p1, p2; 
    int status1, status2;
  
    if (pipe(pipefd) < 0)                                                       // on initialise le pipe tube
    { 
        fprintf(stderr,"mpsh: echec dans pipe.\n"); 
        return -1; 
    } 
    
    p1 = fork(); 
    if (p1 < 0) 
    { 
        fprintf(stderr,"mpsh: echec dans fork.\n"); 
        return -1; 
    } 
  
    if (p1 == 0) 
    { 
        close(pipefd[0]); 
        dup2(pipefd[1], 1);                                                     // le premier fils devient la source d'output
  
        if ( mpsh_execute(args1) < 0)                                           // execute la premiere commande
        { 
            fprintf(stderr,"mpsh: %s: commande introuvable.\n",args1[0]); 
            return -1; 
        } 
        exit(-1);
    } 
    else 
    { 
        p2 = fork(); 
  
        if (p2 < 0) 
        { 
            fprintf(stderr,"mpsh: echec dans fork.\n"); 
            return -1; 
        } 
  
        if (p2 == 0)                                                            
        { 
            close(pipefd[1]); 
            dup2(pipefd[0], 0);                                                 // le deuxieme fils recevera l'output du premier comme input
             
            if ( mpsh_execute(args2) < 0)                                       // execute la deuxieme commande
            { 
                fprintf(stderr,"mpsh: %s: commande introuvable.\n",args2[0]); 
                return -1; 
            } 
            exit(-1);
        } 
        else 
        { 
            close(pipefd[1]);                                                   // restauration des fd par defaut
            close(pipefd[0]);
            waitpid(p1, &status1, WUNTRACED);                                   // attente du fils1
            waitpid(p2, &status2, WUNTRACED);                                   // attente du fils2
            return 0;
        } 
    }
    return -1;
}

int execute_redirections(char** args1, char** args2, char* temp, int pos)
{
    pid_t p;  
    int status;
    
    p = fork();
    if ( p < 0 ) 
    {
        fprintf(stderr, "mpsh: echec dans fork.\n");
        return -1;
    }
     
    else if ( p == 0)                                                           
    {
        if ( temp[pos] == '<' )                                                 
        {
            int fd0;
            if ( ( fd0 = open(args2[0], O_RDONLY, 0) ) == -1)                   // le fils ouvre le fichier
            {
                fprintf(stderr,"mpsh: %s: erreur a la lecture.\n",args2[0]);
                return -1;
            }
            dup2(fd0, 0);                                                       // rendre le fd comme source d'input
            close(fd0);
            args2[0] = NULL;
        }

        else if ( temp[pos] == '>' )
        {
            int fd1;
            if((fd1 = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT,    
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
            { 
                fprintf(stderr,"mpsh: %s: erreur a l'ecriture.",args2[0]);
                return -1;
            }
            dup2(fd1, 1);                                                       // rendre le fd comme source d'output
            close(fd1);
            args2[0] = NULL;
        }
        
        else 
        {
            int fd2;
            if((fd2 = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
            { 
                fprintf(stderr,"mpsh: %s: erreur a l'ecriture.",args2[0]);
                return -1;
            }
            dup2(fd2, 2);                                                       // rendre le fd comme source d'erreur output
            close(fd2);
            args2[0] = NULL;
        }


        if ( mpsh_execute(args1) < 0) 
        { 
            fprintf(stderr,"mpsh: %s: commande introuvable.\n",args1[0]); 
            return -1; 
        } 
        exit(-1);
    } 
    else 
    { 
        do 
        {
            waitpid(p, &status, WUNTRACED);                                     // le pere attend la terminaison du fils
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return 0;
    }  
}

int parse_pipe_redirections(char *temp,int pos,int err)
{
    int j=0;
    int i;
    char* cmd1 = (char*) calloc ( BUFFSIZE,sizeof(char) );
    char* cmd2 = (char*) calloc ( BUFFSIZE,sizeof(char) );
    
    if ( ! err )                                                                // on divise en deux commande selon > < |
    {
        for (i=0; i < pos; i++)
            cmd1[i] = temp[i];
        cmd1[i] = '\0';
        
        for (int i = pos+1; i < strlen(temp); i++)
            cmd2[j++] = temp[i];
        cmd2[j] = '\0';
    }   
    
    else                                                                        // on divise en deux commande selon 2>
    {
        for (i=0; i < pos; i++)
            cmd1[i] = temp[i];
        cmd1[i] = '\0';
        
        for (int i = pos+3; i < strlen(temp); i++)
            cmd2[j++] = temp[i];
        cmd2[j] = '\0';
    }
    
    char** args1 = split_command(cmd1);                                         // transfomer la commande 1 en plusieurs tokens
    char** args2 = split_command(cmd2);                                         // transfomer la commande 2 en plusieurs tokens
    
    if ( temp[pos] == '|' )                                     
        return execute_pipe(args1,args2);                                      // soit en execute les deux commandes en mode pipe
    else 
        return execute_redirections(args1,args2,temp,pos);                     // soit en mode redirection
}

int mpsh_execute(char** args)
{
    int j = 0;
    
    char temp[BUFFSIZE] = "" ;
    for(char* str = args[j]; str != NULL; str = args[j] )
    {
        strcat(temp,str);
        strcat(temp," ");
        j++;
    } 
    temp[strlen(temp)-1] = '\0';

    if (args[0] == NULL)                                                       
        return 0;
    
    int c =  check_operations(args,temp);                                       // si la commandes est un enchainement de plusieurs commandes
    if ( c == 0 || c == -1 ) 
        return -1;
    
    int err=0;
    int pos = find_pipe_redirections(temp,&err);                                // si la commande n'est pas un pipe ou une redirection
    
    if ( pos < 0 )
    {  
        alias *tmp=NULL,*preced=NULL;                                          // chercher la commande dans les alias 
        if ( recherche_alias(args[0],&tmp,&preced) )
        {
            char* value = strdup(tmp->value);
            return mpsh_execute(split_command(value));
        }
        for (int i = 0; i < NUM_BUILTINS; i++)                                  // chercher la commande dans les commandes internes
            if (strcmp(args[0], builtins_name[i]) == 0) 
                return (*builtins_func[i])(args);

        int aff = check_affect(args);                                           // si c'est la commande s'agit d'une affectation
        if ( aff == 1 )
            return -1;
        
        else if ( aff == 0 )
            return 0;          
        
        else
            return mpsh_launch(args);                                          // sinon, lance la commande comme commande externe
    }
    else
        return parse_pipe_redirections(temp,pos,err);                          //sinon si il s'agit d'un pipe ou redirection

}

char** split_command(char *line)
{
    int i=0;
    char** args = (char**) calloc( BUFFSIZE , sizeof(char*) );

    for (char *token = strtok(line,SEP); token != NULL; token = strtok(NULL, SEP))
        args[i++] = strdup(token);
    args[i] = NULL;

    return args;
}

char* mpsh_path(char **argv)                                                    // recuperer le path de l'executable de mpsh
{
    char path_save[BUFFSIZE];
    char abs_exe_path[BUFFSIZE];
    char *p;

    if(!(p = strrchr(argv[0], '/')))                                            
        getcwd(abs_exe_path, sizeof(abs_exe_path));
        
    else
    {
        *p = '\0';
        getcwd(path_save, sizeof(path_save));
        chdir(argv[0]);
        getcwd(abs_exe_path, sizeof(abs_exe_path));
        chdir(path_save);
    }
    
    return strdup(abs_exe_path);
}

int create_mpshrc(char* mpshrc, char** argv)                        
{
    FILE * fp;
    
    if ( ( fp = fopen(mpshrc,"w") ) == NULL)                                   // si y a pas de fichier mpshrc, on le crée
        return -1;
    
    char mpsh[BUFFSIZE];
    char *bash = strdup(getenv("PATH"));
    strcpy(mpsh,mpsh_path(argv));
    
    strcat(mpsh,":");
    strcat(mpsh,bash);

    set_env("CHEMIN",mpsh);
    malloc_var("INVITE",mpsh_prompt());
    
    if ( ( fprintf(fp,"export CHEMIN=%s\n",mpsh) ) == -1 )                      // on ajoute la variable d'environement CHEMIN
        return -1;
    if ( ( fprintf(fp,"INVITE=%s \n",mpsh_prompt()) ) == -1 )                   // on ajoute la variable INVITE
        return -1;
        
    if ( ( fprintf(fp,"# complete gcc c\n") ) == -1 )                           // filtre de completeion pour la commande gcc
        return -1;
    
     if ( ( fprintf(fp,"# complete display jpg png gif\n") ) == -1 )            // filtre de completeion pour la commande display
        return -1;
    
    free(bash);
    fclose(fp);
    return 0;
}

int load_mpshrc(char** argv)
{
    char* line = NULL;
    char mpshrc[BUFFSIZE];
    strcpy(mpshrc,strdup(getenv("HOME")));
    strcat(mpshrc,"/.mpshrc");
    
    size_t len = 0;
    ssize_t read;
    FILE * fp;
    
    if ( ( fp = fopen(mpshrc,"r") ) == NULL)                                    // ouverture du fichier $HOME/.mpshrc
        return create_mpshrc(mpshrc,argv);

    while ( ( read = getline(&line, &len, fp) ) != -1 )                        // si la ligne n'est pas vide ou commentaire 
    {
        if ( line[0] == '#' || strlen(line) == 1 )                            
            continue;
        
        line[strlen(line)-1] = '\0';
        char **args = split_command (line);                                     // alors on l'execute
        
        if ( ( mpsh_execute (args) ) == -1 )
            fprintf(stderr,"mpsh: load_mpshrc: %s: erreur à l'execution de la commande.\n",line);
    }

    if (line)
        free(line);
        
    fclose(fp);
    
    return 0;
}

int initialize_readline()
{
    rl_attempted_completion_function = fileman_completion;

    return 0;
}

char ** fileman_completion (const char *com, int start, int end)
{
    char **matches;
    matches = (char **)NULL; 
    
    if (start == 0)
        matches = rl_completion_matches (com, command_generator);
        
    //rl_ignore_some_completions_function = filter_filename_completion ;
    return (matches);
}

int filter_filename_completion(char **matches) 
{
    
    char* line = NULL;
    char mpshrc[BUFFSIZE];
    strcpy(mpshrc,strdup(getenv("HOME")));
    strcat(mpshrc,"/.mpshrc");
    
    char* extension;
    char* com;
    size_t len = 0;
    FILE * fp;
    
    fp = fopen(mpshrc,"r")      ;                  

    while ( getline(&line, &len, fp)  != -1 )                                   // lecture du fichier mpshrc                
    {
        if ( line[0] == '#' )
        {        
            line[strlen(line)-1] = '\0';
            char **args = split_command (line); 
            if ( ! strcmp(args[1],"complete") )                                 // c'est la ligne est un indice de completion
            {
                com = args[2];                                                  // recupere la commande
                extension = args[3];                                            // recupere l'extension des fichiers a completer
            }
        }
    }
    
    if (line)
        free(line);
        
    fclose(fp);
    
    for (; *matches != NULL; matches++) 
    {
        if ( strstr(*matches,extension) != NULL )                              // si le fichier de completion a l'extension demandé
            printf("%s\t",*matches);                                            // on le laisse ( affiche )

      free(*matches);                                                       
      *matches = NULL;
    }  

    free(com);
    free(extension);
    return 1;
}

char *command_generator (const char *com, int num)
{
    static int indice, len;
    char *completion;

    if (num == 0)
    {
        indice = 0;
        len = strlen(com);
    }

    while (indice < NUM_BUILTINS)
    {
        completion = builtins_name[indice++];

        if (strncmp (completion, com, len) == 0)
            return strdup(completion);
    }

    return NULL;
}
