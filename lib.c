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
#include "lib.h"

alias* als = NULL;                                                              // liste des alias de mpsh
envrn* env = NULL;                                                              // liste des variables environements
variable* vars = NULL;                                                          // liste des variables simples

char *builtins_name[] =                                                         //  liste des noms des commandes intrenes 
{
    "echo",
    "cd",   
    "umask",
    "alias",
    "unalias",
    "exit",
    "pwd",
    "export",
    "type",
    "history"
};

int (*builtins_func[]) (char **) =                                              // les des fonctions des commandes internes
{
    &mpsh_echo,
    &mpsh_cd,
    &mpsh_umask,
    &cmd_alias,
    &cmd_unalias,
    &cmd_exit,
    &cmd_pwd,
    &export,
    &type,
    &history   
};

extern char** split_command(char *line);                                        
extern int mpsh_execute(char** args);

int malloc_env(char* var, char* val)            
{
    envrn* e = (envrn*) calloc(1, sizeof(envrn) );
    if ( e == NULL )
    {
        fprintf(stderr,"mpsh: malloc_env: espace insuffisant.\n");
        return -1;
    }
    
    e->name    = strdup(var);
    e->value   = strdup(val);
    
    if ( env == NULL ) 
    {
        e->next = NULL;
        env = e;
    }
    
    else
    {
        e->next = env ;
        env = e;
    }
    
    return 0;
} 

envrn* recherche_env(char* name)
{
    for (envrn* e = env; e != NULL ; e = e->next)
        if ( ! strcmp(e->name,name) )
            return e;
    return NULL;
}

int set_env(char* name,char* value)                                             // fonction qui affecte une valeur a une variable d'environement
{
    
    envrn *e;
    if ( ( e = recherche_env(name) ) != NULL )
    {
        e->value = strdup(value);                                               // si la valeur existe déja en l'écrase
        return 0;
    }    
    
    return malloc_env(name,value);                                             // sinon on alloue une nouvelle variable
}

void print_env()
{
    for (envrn* e = env; e != NULL ; e = e->next)
        fprintf(stdout,"%s=%s\n",e->name,e->value);
}

char** split(char* str, char delim)
{
    int i;
    char** r = (char**) calloc ( 2 , sizeof(char*) );
    r[0] = (char*) calloc ( strlen(str) , sizeof(char) );
    r[1] = (char*) calloc ( strlen(str) , sizeof(char) );

    for ( i=0; str[i] != delim; i++ )
    {
        r[0][i] = str[i];
    }
    r[0][i] = '\0';
    
    if ( i == strlen(str)-1 )
        r[1] = strdup("");
        
    else   
    {
        int j=0;
        for ( i=i+1; str[i] != '\0'; i++ )
            r[1][j++] = str[i];

        r[1][j] = '\0';
    }
    return r;
}

int export(char** args) 
{   
    int ret = 0;
    
    if ( args[1] == NULL )
    {
        print_env();
        return 0;
    }
    
    int i=1;
    for (char* str  = args[i]; str != NULL; str  = args[i] )                   // boucler sur plusieurs affectation : export a=b c=d ... 
    {  
        i++;
        
        if ( ! isalpha(str[0]) )                                                // identificateur doit commencer avec une lettre
        {
            fprintf(stderr,"mpsh: export: `%c` identificateur invalide.\n",str[0]);
            ret = -1;
            continue;
        }
        
        int index = strcspn(str,"=");                                           

        if ( index == strlen(str) )                                             // si il y a pas d'affectation
        {
            variable* exist;
            if  ( ( exist = recherche_var(str) ) != NULL )                     // si la variable existe comme variable simple
            {
                if ( set_env(str,exist->value) == -1 )                          // alors on la rend comme une variable d'environement
                {
                    fprintf(stderr,"mpsh: export: erreur à l'ajout de %s\n",str);
                    ret = -1;
                }
            }
            
            fprintf(stderr,"mpsh: export: la variable %s n'est existe pas comme variable simple.\n",str);
            ret = -1;
        }

        else                                                                    // sinon si on detecte une affectation
        {
            char delim = '=';
            char** varval = split(str,delim);                                   // on divise selon le '='
            int fin=0;            
         
            if ( varval[1][0] == '"' && varval[1][ strlen(varval[1]) - 1] == '"' )
            {
                varval[1][ strlen(varval[1]) - 1 ] = '\0';
                varval[1]++;
                fin = 1;
            }   
            
            else if ( varval[1][0] == '"' )                                          // si on detecte le '"' 
            {
                while ( args[i] )                                               // on cherche le prochain '"' pour affecter la valeur complete
                {
                    strcat(varval[1]," ");
                    if ( args[i][ strlen( args[i] )  - 1 ] == '"' )             
                    {
                        fin = 1;
                        strncat(varval[1] , args[i], strlen( args[i] )  - 1);
                        i++;
                        break;
                    }
                    else
                    {
                        strcat(varval[1] , args[i]);
                        i++;
                    }
                }

                if ( ! fin )
                {
                    fprintf(stderr,"mpsh: %s: valeur non limitée.\n",str);
                    ret = -1;
                }   
                varval[1]++; 
            }
            
            else if ( varval[1][0] == '\'' && varval[1][ strlen(varval[1]) - 1] == '\'' )
            {
                varval[1][ strlen(varval[1]) - 1 ] = '\0';
                varval[1]++;
                fin = 1;
            }   
            
            else if ( varval[1][0] == '\'' )                                    // la meme chose qu'avant juste avec le ' au lieu de "
            {
                i++;
                while ( args[i] )
                {
                    strcat(varval[1]," ");
                    if ( args[i][ strlen( args[i] )  - 1 ] == '\'' )
                    {
                        fin = 1;
                        strncat(varval[1] , args[i], strlen( args[i] )  - 1);
                        i++;
                        break;
                    }
                    else
                    {
                        strcat(varval[1] , args[i]);
                        i++;
                    }
                }

                if ( ! fin )
                {
                    fprintf(stderr,"mpsh: %s: valeur non limitée.\n",str);
                    return 1;
                }   
                varval[1]++; 
            }
            
            if ( set_env(varval[0],varval[1]) == -1 )                           // on ajoute la nouvelle variable d'environement
            {
                fprintf(stderr,"mpsh: export: %s n'est pas ajouté aux variables d'environemment.\n",str);
                ret = -1;
            }
        } 
    }
    return ret;    
}

int check_affect(char **args)
{
    int i = 0;
    
    char temp[BUFFSIZE] = "" ;
    for(char* str = args[i]; str != NULL; str = args[i] )
    {
        strcat(temp,str);
        strcat(temp," ");
        i++;
    } 
    temp[strlen(temp)-1] = '\0';
    
    if ( strcspn(temp,"=") == strlen(temp) )                                    // si on trouve pas de '=', donc pas d'affectation
        return -1;
    
    i = 0;
        
    for ( char* str = args[i]; str != NULL; str = args[i] )
    {        
        int index = strcspn(str,"=");

        if ( index == strlen(str) )                                             // si le '=' n'est pas dans le premier token, donc erreur
        {
            fprintf(stderr,"mpsh: %s: commande introuvable.\n",args[i]);
            return 1;
        }
        
        else
        {
            if ( index == 0 && i > 0 )                                          // si le '='  est le debut du token, donc erreur
            {
                fprintf(stderr,"mpsh: %s: commande introuvable.\n",args[i-1]);
                return 1;
            }
            
            
            if ( ! isalpha(str[0])   )                                          // si l'identificateur ne commence pas avec une lettre, erreur 
            {
                fprintf(stderr,"mpsh: %s: commande introuvable.\n",str);
                return 1;
            }
            
            char delim = '=';
            envrn* e;
            char** varval = split(str,delim);
            int fin=0;
            
            if ( strstr(temp,"\"") || strstr(temp,"'") )
            {

                if ( varval[1][0] == '"' && varval[1][ strlen(varval[1]) - 1] == '"' )
                {
                    varval[1][ strlen(varval[1]) - 1 ] = '\0';
                    varval[1]++;
                    fin = 1;
                }   
                 
                else if ( varval[1][0] == '"' )                                     // affectation qui contient '"' 
                {

                    i++;
                    while ( args[i] )                                               // on cherche le prochain '"', pour affecter la valeur
                    {
                        strcat(varval[1]," ");
                        if ( args[i][ strlen( args[i] )  - 1 ] == '"' )
                        {
                            fin = 1;
                            strncat(varval[1] , args[i], strlen( args[i] )  - 1);
                            i++;
                            break;
                        }
                        else
                        {
                            strcat(varval[1] , args[i]);
                            i++;
                        }
                    }

                    if ( ! fin )
                    {
                        fprintf(stderr,"mpsh: %s: valeur non limitée.\n",str);
                        return 1;
                    }   
                    varval[1]++; 
                }
                
                else if ( varval[1][0] == '\'' && varval[1][ strlen(varval[1]) - 1] == '\'' )
                {
                    varval[1][ strlen(varval[1]) - 1 ] = '\0';
                    varval[1]++;
                    fin = 1;
                }   
                
                else if ( varval[1][0] == '\'' )                                    // on fait la meme chose qu'avant, au lieu de " on a '
                {
                    i++;
                    while ( args[i] )
                    {
                        strcat(varval[1]," ");
                        if ( args[i][ strlen( args[i] )  - 1 ] == '\'' )
                        {
                            fin = 1;
                            strncat(varval[1] , args[i], strlen( args[i] )  - 1);
                            i++;
                            break;
                        }
                        else
                        {
                            strcat(varval[1] , args[i]);
                            i++;
                        }
                    }

                    if ( ! fin )
                    {
                        fprintf(stderr,"mpsh: %s: valeur non limitée.\n",str);
                        return 1;
                    }   
                    varval[1]++; 
                }
            }

            if ( ( e =  recherche_env(varval[0]) ) != NULL )                   // si la variable est une variable d'environement
                e->value = strdup(varval[1]);                                   // alors, on change sa valeur
            
            else 
            {
                variable* val;
                if ( ( val = recherche_var(varval[0]) ) != NULL )              // si la variable est une variable simple
                    val->value = strdup(varval[1]);                             // alors, on change sa valeur
                else
                    malloc_var(varval[0],varval[1]);                            // sinon, on aloue une nouvelle variable
            }
        }
        i++;
    }

    return 0; 
}

int type(char **args)
{
    if ( args[1] == NULL )
    {
        fprintf(stderr,"mpsh: type: argument manquant.\nUsage : type name [name ...].\n");
        return -1;
    }
    
    int i=1;
    for (char* cmd = args[i]; cmd != NULL ; cmd = args[i])                     
    {
        int is_alias = 0;
        int is_builtin = 0;
        i++;
        
        for ( alias* tmp = als; tmp != NULL; tmp = tmp->next )                 // d'abord vérifier si la commande est un alias
        {
            if ( ! strcmp(tmp->name,cmd) )
            {
                fprintf(stdout,"%s est un alias vers `%s`\n",tmp->name,tmp->value);
                is_alias = 1;
                break;   
            }
        }
        
        if ( is_alias )
            continue;
    
        int j = 0;
        for (char* tmp = builtins_name[j]; tmp != NULL; tmp = builtins_name[j])// puis si c'est une commande interne
        {
           if ( ! strcmp(tmp,cmd) )
            {
                fprintf(stdout,"%s est une commande interne mpsh.\n",tmp);
                is_builtin = 1;
                break;  
            } 
            j++;
        }
        
        if ( is_builtin )
            continue;

        char* ch = recherche_env("CHEMIN")->value;                              // sauvegarde du contexte des variable d'env mpsh
        char* chemin = transform_chemin(ch);
        char* old_path = strdup(getenv("PATH"));                                // sauvegarde du contexte des variable d'env bash
        setenv("PATH",chemin,1);                                                // affectation d'un nouveau chemin pour le bash
        
        pid_t pid = fork();
        int status;

        if ( pid == -1 ) 
        {
            fprintf(stderr,"mpsh: type: echec dans fork.\n");
            return -1;
        }

        else if ( pid == 0 )                                                   // forker un fils pour voir si la commande existe ailleur 
        {            
            char* temp = strdup("type ");
            strcat(temp,cmd);
            
            char* cmd = strdup("sh");
            char* args[] = {"sh","-c",temp,NULL};
            
            if ( execvp(cmd, args) < 0) 
            {
                fprintf(stderr,"mpsh: %s: commande introuvable.\n",args[0]);     
                return -1;
            }
            exit(-1);       
        }        
        else 
        { 
            do 
            {
                setenv("PATH",old_path,1);                                      // restauration du contexte bash
                set_env("CHEMIN",chemin);                                       // restauration du contexte mpsh
                waitpid(pid, &status, WUNTRACED);                               // attente du fils forké
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            
            return 0;
        } 
    }

    return 0;
}

int count_lines (FILE* fp)
{
    int count = 0;
    rewind(fp);
    
    for (char c = getc(fp); c != EOF; c = getc(fp)) 
        if (c == '\n') 
            count = count + 1; 
            
    return count;
}

int string_num(char *s,int base, int *result )
{ 
    char *eptr;
    errno = 0;
    *result = strtol(s, &eptr, base);

    if ( *eptr != '\0' || errno == ERANGE || ( errno != 0 && *result == 0 ) )     
        return -1;

    return 0;           
}

int history (char **args)
{
    char* arg = args[1];
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int i =  1;
    
    char* mpsh_history = strdup(getenv("HOME"));
    strcat(mpsh_history,"/.mpsh_history");              
    FILE* fp = fopen(mpsh_history,"r");                                         // lecture du fichier qui stock les commandes $HOME/.mpsh_history
    
    if ( fp == NULL )
        return -1;
    
    if ( arg == NULL )                                                         // si la commande est lancée sans argument
    {        
        int i =  1;     
        while ( ( read = getline(&line, &len, fp) ) != -1 ) 
            if ( line[0] != '#' || strlen(line) > 1 )                           // si la ligne n'est pas un commentaire et n'est pas vide        
                fprintf(stdout,"%4d   %s",i++,line);                            // on l'affiche

        if (line)  
            free(line); 
            
        fclose(fp);
        return 0;
    }
    
    int index=0;
    int base = 10;
    
    if ( string_num(arg,base,&index) == -1 )
    {
        fprintf(stderr,"mpsh: history: %s n'est pas une valeur numerique.\n",arg);
        return -1;
    }   
    
   
    if ( index >= 0 )                                                            // si l'argument est un chiffre positive
    {       
        while ( ( read = getline(&line, &len, fp) ) != -1 ) 
        {
            if ( i++ ==  index ) 
            {
                fprintf(stdout,"%s",line); 
                line[strlen(line)-1] = '\0';
                char **args = split_command (line);                             // on cherche la ligne 
                return mpsh_execute (args);                                    // on re-execute la commande
            }     
        }
        
        fprintf(stderr,"mpsh: history: indice hors limite.\n");
        fclose(fp);
        return -1;
    }
    
    else                                                                        // si l'argument est un chiffre negative
    {
        int nb_lines = count_lines(fp);
        rewind(fp);
        int limit = nb_lines + index;
    
        while ( ( read = getline(&line, &len, fp) ) != -1 ) 
            if ( i++ > limit )                                                  
                fprintf(stdout,"%4d   %s",i-1,line);                            // on affiche les derniere n lignes
                
        fclose(fp);
        return 0;        
    }
    
}

void affiche_alias()
{
	alias *tmp = als;
	
	while ( tmp != NULL )
	{
		fprintf(stdout, "alias %s='%s'\n",tmp->name,tmp->value);
		tmp=tmp->next;
	} 
}

int recherche_alias(char name[],alias** elem,alias** preced)
{
	*preced = NULL;
	*elem = als;
	
	while ( *elem!=NULL && strcmp((*elem)->name,name) < 0 ) 
	{	
		*preced=*elem;
		*elem=(*elem)->next;
	}
	if ( *elem!=NULL && strcmp( (*elem)->name, name ) == 0 )	
		return 1;
	else 
		return 0;
		
}

void ajout_alias(alias *elem)
{
	alias *tmp = NULL, *preced = NULL;
	int trouve = recherche_alias(elem->name,&tmp,&preced);
	
	if ( trouve )
		tmp->value=elem->value;
	
	else
	{
		
		if(preced==NULL)
		{
			als=elem;
			elem->next=tmp;
		}
		else
		{
			preced->next=elem;
			elem->next=tmp;	
		}
	}

}

int valid_name(char *name)
{
	int i=0;
	
	if(name[i]=='$' || name[strlen(name)-1]=='$' ||name[i]=='/' || name[strlen(name)-1]=='/')
		return 0;
		
	while ( i<strlen(name)||isalnum(name[i])!=0 || \
	        name[i] ==':' ||name[i]=='.' ||name[i]=='_' || \
	        name[i]=='~' || name[i]=='-' || name[i]=='?' || name[i]=='$')
    {	
		i++;
	}
	
	if ( i<strlen(name) )
		return 0;
	else
		return 1; 
}

int cmd_alias(char* cmd[]){
	int retour=0;

	//cmd = "alias"
	if(cmd[1]==NULL)
	{
		affiche_alias();
		return retour;
  	}
	
	//cmd = "alias + params "
	int i=1;
	alias *tmp=NULL,*preced=NULL;
	for(char* param=cmd[i];cmd[i]!=NULL;param=cmd[i])
	{
		//cas affichage
		if(strchr(param,'=')==NULL || param[0]=='=')
		{
			//alias existe deja
			if(recherche_alias(param,&tmp,&preced))
				fprintf(stdout, "alias %s='%s'\n",tmp->name,tmp->value);
			//alias n'existe pas 
			else
			{
				fprintf(stderr,"mpsh: alias: %s: introuvable.\n",param);
				retour=-1;
			}			
		}
		//cas ajout
		else
		{
			char** tab=split(param,'=');
			if(valid_name(tab[0]))
			{

				if(strchr(tab[1],'"')!=NULL)
				{
					tab[1]=(tab[1])+1;
					int ouvert=1;
					
					if(strchr(tab[1],'"')!=NULL)
					{
            			ouvert=0;
                    }
                    
					while(ouvert)
					{
						i++;
						strcat(tab[1]," ");
						strcat(tab[1],cmd[i]);
						if(strchr(cmd[i],'"')!=NULL)
							ouvert=0;					
					}
					tab[1][strlen(tab[1])-1]='\0';	
				}
                else if(strchr(tab[1],'\'')!=NULL)
				{
					tab[1]=(tab[1])+1;
					int ouvert=1;
					while(ouvert)
					{
						i++;
						strcat(tab[1]," ");
						strcat(tab[1],cmd[i]);
						if(strchr(cmd[i],'\'')!=NULL)
							ouvert=0;					
					}
					tab[1][strlen(tab[1])-1]='\0';	
				}
                
				tmp= (alias*) calloc(1,sizeof(alias));
				tmp->name=tab[0];
				tmp->value=tab[1];
				ajout_alias(tmp);
			}
			else
			{

				fprintf(stderr,"-mpsh: alias: %s: invalid alias name\n",param);
				retour=-1;
			}
		}
		i++;
	}
	return retour;
}

int cmd_unalias(char** cmd)
{
	int i=1;
	int retour=0;
	
	for(char* param=cmd[i];cmd[i]!=NULL;param=cmd[i])
	{
		alias *elem=NULL,*preced=NULL;
		if(recherche_alias(param,&elem,&preced))
		{
			if(preced==NULL)
				als=elem->next;
	
			else
				preced->next=elem->next;

			free(elem);
		}
		else
		{
			fprintf(stderr,"mpsh: unalias: %s: alias non trouvé.\n",param);
			retour=-1;
		}
		
		i++;
	}
	return 
	    retour;
}

int cmd_exit(char** cmd)                                                        
{
	if ( cmd[1] )                                                               // s'il existe un argument
	{
	    int base = 16;
	    int exit_val;
	    if ( string_num (cmd[1],base,&exit_val) == -1 )                         // si ce n'est pas un chiffre, donc erreur
	    {
	        fprintf(stderr,"mpsh: exit: %s n'est pas une valeur numerique\n",cmd[1]);
	        return -1;
	    }
	    
        exit (exit_val);                                                        // sinon, on exit avec ma valeur de l'arguement
	}
	
	else
	    exit( atoi( recherche_var("?")->value ) );                              // si y a pas d'argument, on exit avec la derniere valeur
	
}

int cmd_pwd(char** cmd) 
{
	char cwd[1024];
	
	if ( getcwd(cwd, sizeof(cwd) ) == NULL )
	{
        fprintf(stderr,"mpsh: pwd: echec dans pwd.\n");
		return -1;
	}
	else
	{
		fprintf(stdout,"%s\n", cwd);
		return 0;
	}
}

void sub_transf(char *token, char *path){
    DIR *dir=NULL;
	struct dirent* dirFile = NULL; 

	dir=opendir(token);
	if(dir==NULL)
		return;
		
	while ((dirFile = readdir(dir)) != NULL)
	{
		if(dirFile->d_type == DT_DIR && strcmp(dirFile->d_name,"..") != 0 && strcmp(dirFile->d_name,".") !=0 )
		{
			strcat(path,token);
			strcat(path,"/");
			strcat(path,dirFile->d_name);
			strcat(path,":");
			char *subToken=(char *)calloc( BUFFSIZE ,sizeof(char));
			strcpy(subToken,token);
			sub_transf(strcat(strcat(subToken,"/"),dirFile->d_name),path);				
		}
	}
	closedir(dir);
}

char* transform_chemin(char* chemin)
{
	char *path= (char*)calloc(BUFFSIZE, sizeof(char));
	char *token=strtok(chemin,":");

	while (token!=NULL)
	{
  		char *double_slash=NULL;
		double_slash=strstr(token,"//");
		//Ne se termine pas avec "//" 
		if(double_slash==NULL)
		{
			strcat(path,token);
			strcat(path,":");
		}
		else
		{
			double_slash[0]='\0';
			strcat(path,token);
			strcat(path,":");
			sub_transf(token,path);	
		}
		token=strtok(NULL,":");
	}
	path[strlen(path)-1]='\0';
	return path;
}

int malloc_var(char* var, char* val)            
{
    variable* v = (variable*) calloc(1, sizeof(variable) );
    if ( v == NULL )
    {
        fprintf(stderr,"mpsh: malloc_env: espace insuffisant.\n");
        return -1;
    }
    
    v->name    = strdup(var);
    v->value   = strdup(val);
    
    if ( vars == NULL ) 
    {
        v->next = NULL;
        vars = v;
    }
    
    else
    {
        v->next = vars ;
        vars = v;
    }
    
    return 0;
}

variable* recherche_var(char* var)
{
    variable* temp= vars;
    while(temp!=NULL)
    {
        if(strcmp(temp->name,var)==0) 
            return temp;
        else 
            temp=temp->next;
    }
    
    return NULL;
}

int mpsh_echo (char **args)
{
    int l=1;
    for (char* s = args[l]; s != NULL; s = args[l] )
    {
        l++;
        char var[BUFFSIZE];
        int i=0;  
        int j=0;

        while (s[i]==' ' || s[i]=='\n' || s[i]=='\t')
            i++;  

        while(s[i]!='\0')
        {
            if (s[i]!=' ' && s[i]!='\n' && s[i]!='\t')
            { 

                if(s[i]!='$') 
                {
                    putc(s[i],stdout); 
                    i++;
                }
                else 
                { 
                    if ( s[i+1] == '?' )
                    {
                                fprintf(stdout,"%s",recherche_var("?")->value);
                                i++;  
                    }
                    i++;
                    if(isdigit(s[i])) 
                        i++;
                    else 
                    {
                        j=0;
                        while(isalnum(s[i]))
                        {
                            var[j]=s[i];
                            i++;
                            j++;
                        } 
                        var[j]='\0';
                        envrn* e = recherche_env(var);
                        variable* v = recherche_var(var);
                        if ( e != NULL )
                            fprintf(stdout,"%s",e->value);
                        else if (v != NULL) 
                            fprintf(stdout,"%s",v->value);
                    } 
                }   
            }
            else  
            { 
                while ( s[i]==' ' || s[i]=='\n' || ( s[i]=='\t' && s[i] ) )
                    i++; // si on trouve des espaces on en laisse un
                fprintf(stdout," ");  
            }
        }
        fprintf(stdout," ");  
    }
    
    fprintf(stdout,"\n");
 
    return 0;
}

int mpsh_cd(char **args)
{
    int i;
    for ( i=0; args[i] != NULL; i++ );
    
    if ( i > 2 )
    {
        fprintf(stderr,"mpsh: cd: trop d'arguments.\n");
        return -1;
    }
  
    char* path = args[1];
    
    if ( ! args[1] ) 
    {
      chdir(strdup(getenv("HOME")));
      return 0;
    } 

    else 
    {
        if (chdir(path) == 0)
            return 0;
        else
        {
          fprintf(stderr,"mpsh: cd: %s: Aucun fichier ou dossier de ce type.\n",path);
          return -1;
        }
    }
}

int mpsh_umask(char **args)
{ 
    int result;
    int base = 8;
  
    if ( ! args[1] )
    {
        fprintf(stderr,"mpsh: umask: argument manquant.\nUsage : umask 'valeur d'umask en octal'.\n");
        return -1;
    } 
    
    char*s = args[1];
   
    if ( strlen(s) > 3 ) 
    {
        fprintf(stderr,"mpsh: umask: %s: nombre octal hors limite.\n",s); 
        return -1;
    }
      
    if ( string_num(s,base,&result) == -1 )
    {
        fprintf(stderr,"mpsh: umask: %s: nombre octal hors limite : %d\n",s,errno);
        return -1;
    }   
    
    umask( (mode_t) result);  
    return 0;
}
