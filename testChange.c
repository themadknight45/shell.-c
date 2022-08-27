
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#define clear() printf("\033[H\033[J")
#define LIMIT 256 // max number of tokens for a command
#define MAXLINE 1024 // max number of characters from user input
#define TRUE 1
#define FALSE !TRUE
char testCmd[100];
char usrCmd[100];
// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;
static int err=0;
static char* currentDirectory;
extern char** environ;
static int h_index=0;
struct sigaction act_child;
struct sigaction act_int;
int no_reprint_prmpt;
pid_t pid;
const char *cmd_history[1024];
const pid_t *child_process[1024];
static int process_index=0;

int valid(char testCmd[256],char usrCmd[256]){
	strncat(testCmd, usrCmd, 93);
	size_t size = 0;
	char path[100];
	FILE *p = popen( testCmd , "r");
	if(p) {
		while(fgets(path, sizeof(path), p) != NULL) {
					size= strlen(path);
		}
	}
	if(size==0){
		return 0;
	}
	return 1;
}
/**
 * SIGNAL HANDLERS
 */
// signal handler for SIGCHLD */
void signalHandler_child(int p);
// signal handler for SIGINT
void signalHandler_int(int p);
void init(){
		// See if we are running interactively
        clear();
		// kill(0, SIGKILL);
        GBSH_PID = getpid();
        // The shell is interactive if STDIN is the terminal  
        GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);  

		if (GBSH_IS_INTERACTIVE) {
			// Loop until we are in the foreground
			while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
					kill(GBSH_PID, SIGTTIN);             
	              
	              
	        // Set the signal handlers for SIGCHILD and SIGINT
			act_child.sa_handler = signalHandler_child;
			act_int.sa_handler = signalHandler_int;			
			
			/**The sigaction structure is defined as something like
			
			struct sigaction {
				void (*sa_handler)(int);
				void (*sa_sigaction)(int, siginfo_t *, void *);
				sigset_t sa_mask;
				int sa_flags;
				void (*sa_restorer)(void);
				
			}*/
			
			sigaction(SIGCHLD, &act_child, 0);
			sigaction(SIGINT, &act_int, 0);
			
			// Put ourselves in our own process group
			setpgid(GBSH_PID, GBSH_PID); // we make the shell process the new process group leader
			GBSH_PGID = getpgrp();
			if (GBSH_PID != GBSH_PGID) {
					printf("Error, the shell is not process group leader");
					exit(EXIT_FAILURE);
			}
			// Grab control of the terminal
			tcsetpgrp(STDIN_FILENO, GBSH_PGID);  
			
			// Save default terminal attributes for shell
			tcgetattr(STDIN_FILENO, &GBSH_TMODES);

			// Get the current directory that will be used in different methods
			currentDirectory = (char*) calloc(1024, sizeof(char));
        } else {
                printf("Could not make the shell interactive.\n");
                exit(EXIT_FAILURE);
        }
}

/**
 * SIGNAL HANDLERS
 */

/**
 * signal handler for SIGCHLD
 */
void signalHandler_child(int p){
	/* Wait for all dead processes.
	 * We use a non-blocking call (WNOHANG) to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. */
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
	printf("\n");
}

/**
 * Signal handler for SIGINT
 */
void signalHandler_int(int p){
	// We send a SIGTERM signal to the child process
	if (kill(pid,SIGTERM) == 0){
		printf("\nProcess %d received a SIGINT signal\n",pid);
		no_reprint_prmpt = 1;			
	}else{
		printf("\n");
	}
}

/**
 *	Displays the prompt for the shell
 */
void shellPrompt(){
	printf("%s~$ ",getcwd(currentDirectory, 1024));
}


/**
 * Method used to manage the environment variables with different
 * options
 */ 
int manageEnviron(char * args[], int option){
	char **env_aux;

	switch(option){
		// Case 'environ': we print the environment variables along with
		// their values
		case 0: 
			for(env_aux = environ; *env_aux != 0; env_aux ++){
				printf("%s\n", *env_aux);
			}
			break;
		// Case 'setenv': we set an environment variable to a value
		case 1: 
			if((args[1] == NULL) && args[2] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			
			// We use different output for new and overwritten variables
			if(getenv(args[1]) != NULL){
				// unsetenv(args[1]);
				printf("%s", "The variable has been overwritten\n");
			}else{
				printf("%s", "The variable has been created\n");
			}
			// printf("%s",getenv(args[1]));
			// If we specify no value for the variable, we set it to ""

			if (args[2] == NULL){
				setenv(args[1], "", 1);
			// We set the variable to the given value
			}else{
				char *a=args[2];
				printf("%s",args[2]);
				int len = strlen(a);
				char buf[len+2];
				strcpy(buf, a);
				buf[len] = 0;
				// buf[len + 1] = 0;
				char *b=strdup(buf);
				setenv(args[1], b, 1);
			}
			break;
		// Case 'unsetenv': we delete an environment variable
		case 2:
			if(args[1] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			if(getenv(args[1]) != NULL){
				unsetenv(args[1]);
				printf("%s", "The variable has been erased\n");
			}else{
				printf("%s", "The variable does not exist\n");
			}
		break;
		case 3:
			if(getenv(args[1])==NULL){
				printf("%s","No such variable exist\n");
			}
			else{
				printf("%s\n",getenv(args[1]));
			}
		break;
			
	}
	return 0;
}
 
/**
* Method for launching a program. It can be run in the background
* or in the foreground
*/ 
void launchProg(char **args, int background){	 
	 if((pid=fork())==-1){
		 printf("Child process could not be created\n");
		 return;
	 }
	 // pid == 0 implies the following code is related to the child process
	if(pid==0){
		// We set the child to ignore SIGINT signals (we want the parent
		// process to handle them with signalHandler_int)	
		
		signal(SIGINT, SIG_IGN);
		// We set parent=<pathname>/simple-c-shell as an environment variable
		// for the child
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		// If we launch non-existing commands we end the process
        if(strcmp(args[0],"&") == 0){
            char *args_aux[256];int j=1;
            while ( args[j] != NULL){
            args_aux[j-1] = args[j];
            j++;
	        }
            args_aux[j-1]=NULL;
            execvp(args_aux[0],args_aux);
            printf("Command not found");
			kill(getpid(),SIGTERM);
        }
        else{
            err=execvp(args[0],args);
			printf("Command not found");
			kill(getpid(),SIGTERM);
        }
	 }
     
        // printf("%i",err);
		child_process[process_index]=pid;
		process_index++;
		// printf("%d %d",pid,process_index);
	 if (background == 0){
		 waitpid(pid,NULL,0);
	 }else{
		 printf("Process created with PID: %d\n",pid);
	 }	 
}
 

/**
* Method used to manage pipes.
*/ 
void pipeHandler(char * args[]){
	// File descriptors
	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int err = -1;
	int end = 0;
	
	// Variables used for the different loops
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
	// First we calculate the number of commands (they are separated
	// by '|')
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	
	// Main loop of this method. For each command between '|', the
	// pipes will be configured and standard input and/or output will
	// be replaced. Then it will be executed
	while (args[j] != NULL && end != 1){
		k = 0;
		// We use an auxiliary array of pointers to store the command
		// that will be executed on each iteration
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				// 'end' variable used to keep the program from entering
				// again in the loop when no more arguments are found
				end = 1;
				k++;
				break;
			}
			k++;
		}
		// Last position of the command will be NULL to indicate that
		// it is its end when we pass it to the exec function
		command[k] = NULL;
		j++;		
		
		// Depending on whether we are in an iteration or another, we
		// will set different descriptors for the pipes inputs and
		// output. This way, a pipe will be shared between each two
		// iterations, enabling us to connect the inputs and outputs of
		// the two different commands.
		if (i % 2 != 0){
			pipe(filedes); // for odd i
		}else{
			pipe(filedes2); // for even i
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){
			// If we are in the first command
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}
			// If we are in the last command, depending on whether it
			// is placed in an odd or even position, we will replace
			// the standard input for one pipe or another. The standard
			// output will be untouched because we want to see the 
			// output in the terminal
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ // for even number of commands
					dup2(filedes2[0],STDIN_FILENO);
				}
			// If we are in a command that is in the middle, we will
			// have to use two pipes, one for input and another for
			// output. The position is also important in order to choose
			// which file descriptor corresponds to each input/output
			}else{ // for odd i
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ // for even i
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}		
		}
				
		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(filedes[0]);
			}else{					
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(filedes2[0]);
				close(filedes[1]);
			}else{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
}
			
/**
* Method used to handle the commands entered via the standard input
*/ 
int commandHandler(char * args[]){
	// int x=getpid();
	// printf("%i",x);
    // printf("%s\n",args[0]);
	int i = 0;
	int j = 0;
	int background = 0;
	char *args_aux[256];
	while ( args[j] != NULL){
        
		args_aux[j] = args[j];
		j++;
	}
	args_aux[j]=NULL;
    char *s;
	s=strchr(args_aux[0],'=');
	if(s!=NULL){
		char *envi =   "setenv ";
		char *arguments= args_aux[0];
		char command[MAXLINE];
		int k=0;
		for(k=0;k<7;k++){
			command[k]=envi[k];
			// printf("%c",command[k]);
		}
		
		while(arguments[k-7]!=NULL){
			if(arguments[k-7]=='='){
				command[k]=' ';
			}
			else{
				command[k]=arguments[k-7];
			}
			k++;
		}
		char * tokens[LIMIT]; 
		int numTokens;
		if((tokens[0] = strtok(command," \n\t")) == NULL) return 1;
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
		manageEnviron(tokens,1);
		return 1;
	}
	 
	if(strcmp(args_aux[0],"echo") == 0 )	{
		char *s;
		s=strchr(args_aux[1],'$');
		if(s!=NULL){
			char *envi =   "getenv ";
			char *arguments= args[1];
			char command[MAXLINE];
			int k=0;
			for(k=0;k<7;k++){
				command[k]=envi[k];
			}
			while(arguments[k-6]!=NULL){
				command[k]=arguments[k-6];
				k++;
			}
			char * tokens[LIMIT]; 
			int numTokens;
			if((tokens[0] = strtok(command," \n\t")) == NULL) return 1;
			numTokens = 1;
			while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
			manageEnviron(tokens,3);
			return 1;
		}
	} 

	// 'exit' command quits the shell
	if(strcmp(args[0],"exit") == 0) exit(0);
	else if (strcmp(args[0],"clear") == 0) system("clear");
	// 'environ' command to list the environment variables

	else if (strcmp(args[0],"env") == 0){
		manageEnviron(args,0);
	}
	// 'setenv' command to set environment variables
	else if (strcmp(args[0],"setenv") == 0) manageEnviron(args,1);
	// 'unsetenv' command to undefine environment variables
	else if (strcmp(args[0],"unsetenv") == 0) manageEnviron(args,2);
	else if (strcmp(args[0],"getenv") == 0) manageEnviron(args,3);
    else if (strcmp(args[0],"cmd_history")==0){
        //printf("%i",h_index);
        for(int i=0;i<5;i++){
            if(h_index-i-1<0)break;
            printf("%s",cmd_history[h_index-i-1]);
            printf("\n");
        }
        return 1; 
    }
	else if(strcmp(args[0],"ps_history")==0){
		for(int i=process_index-1;i>=0;i--){
			printf("%ld",child_process[i]);
			if(kill(child_process[i],0)==0){
				printf(" Running \n");
			}
			else{
				printf(" Stopped \n");
			}
		}
	}
    
	else{
        // printf("%s\n",args[0]);
		while (args[i] != NULL ){
			if (strcmp(args[i],"&") == 0){
				background = 1;
			}else if (strcmp(args[i],"|") == 0){
				pipeHandler(args);
				return 1;
			}
			i++;
		}
		args_aux[i] = NULL;
		char testCmd[256];char usrCmd[256];
		if (strcmp(args[0],"&") == 0){
			strcpy(usrCmd,args[1]);
		}
		else{
			strcpy(usrCmd,args[0]);
		}
		strcpy(testCmd,"which ");
       
		if(valid(testCmd,usrCmd)==0){
			printf("CommandNotFound \n\n");
			return 1;
		}
        //  printf("%s\n",args[0]);
		launchProg(args,background);
	}
return 1;
}


/**
* Main method of our shell
*/ 
int main(int argc, char *argv[], char ** envp) {
	
	
	char line[MAXLINE]; 
	char * tokens[LIMIT]; 
	int numTokens;
	
	no_reprint_prmpt = 0; 	
	pid = -10; 
	init();
	environ = envp;
	setenv("shell",getcwd(currentDirectory, 1024),1);
	while(TRUE){
		// int x=getpid();
		// printf("%ihduufh",x);
        err=0;
		if (no_reprint_prmpt == 0) shellPrompt();
		no_reprint_prmpt = 0;
        memset ( line, '\0', MAXLINE);
		fgets(line, MAXLINE, stdin);
		if(line[0]=='\0')return 1;
		int index=0;int indexl=0;
        // printf("%s",line);
        size_t size = strlen(line)-1;
        //printf("%i",strlen(line));
		while(indexl<size){
			int u=indexl;
			while(u<size && line[u]!=' '){
				u++;
			}
			char temp[u-indexl+1];

			for(int v=indexl;v<u;v++){
				temp[v-indexl]=line[v];
			}
			temp[u]='\0';

			tokens[index]=strdup(temp);
            // printf("%s",tokens[index]);
			index++;
			if(u==size){
				tokens[index]=NULL;break;
			}
			indexl=u+1;
		}
        cmd_history[h_index]=strndup(line, size);
        h_index++;
		commandHandler(tokens);
	}          

	exit(0);
}