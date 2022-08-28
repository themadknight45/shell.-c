#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define clear() printf("\033[H\033[J")
#define MAX_TOKENS 512 // MAX TOKEN FROM USER
#define MAX_CHARS 2000  //MAX CHARXS FROM USER
char testCmd[100];
char usrCmd[100];
static pid_t SHELL_PID;
static pid_t SHELL_GROUPID;
static int INTERACTIVE_SHELL;
static struct termios GBSH_TMODES;
static int ERROROR=0;
static char* PR_DIRECTORY;
extern char** environ;
static int h_index=0;
struct sigaction child_sig_handler; //CHILD SIGLE HANDLER
struct sigaction INT_Handler; //INTERRUPT HANDLER
int shell_print; //METHOD FOR PRINTING THE SHELL PATH
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
void signalHandler_child(int p);
void signalHandler_int(int p);
void init(){
        clear();
		INTERACTIVE_SHELL = isatty(STDIN_FILENO); 
        SHELL_PID = getpid();
		if (INTERACTIVE_SHELL) {
			while (tcgetpgrp(STDIN_FILENO) != (SHELL_GROUPID = getpgrp()))
					kill(SHELL_PID, SIGTTIN);             
			child_sig_handler.sa_handler = signalHandler_child;
			INT_Handler.sa_handler = signalHandler_int;	
            sigaction(SIGINT, &INT_Handler, 0);
			sigaction(SIGCHLD, &child_sig_handler, 0);
            SHELL_GROUPID = getpgrp();
			setpgid(SHELL_PID, SHELL_PID); 
			if (SHELL_PID != SHELL_GROUPID) {
					printf("Error, the shell cound not be process group leader");
					exit(EXIT_FAILURE);
			}
			tcsetpgrp(STDIN_FILENO, SHELL_GROUPID);  
			tcgetattr(STDIN_FILENO, &GBSH_TMODES);

			PR_DIRECTORY = (char*) calloc(1024, sizeof(char));
        } else {
                printf("Failed iteractive shell\n");
                exit(EXIT_FAILURE);
        }
}

void signalHandler_child(int p){
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
	// printf("\n");
}

void signalHandler_int(int p){
	if (kill(pid,SIGTERM) == 0){
		printf("\nProcess %d received a SIGINT signal\n",pid);
		shell_print = 1;			
	}else{
		printf("\n");
	}
}

void shellPrompt(){
	printf("%s~$ ",getcwd(PR_DIRECTORY, 1024));
}


int manageEnviron(char * args[], int option){
	char **env_aux;
	
		if(option==0){
			for(env_aux = environ; *env_aux != 0; env_aux ++){
				printf("%s\n", *env_aux);
			}
        }

		else if(option==1){ 
			if((args[1] == NULL) && args[2] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			if (args[2] == NULL){
				setenv(args[1], "", 1);
			}else{
				setenv(args[1], args[2], 1);
			}
        }
		else if(option==2){
			if(args[1] == NULL){
				printf("\n%s","Not enought input arguments\n");
				return -1;
			}
			if(getenv(args[1]) == NULL){
				printf("%s", "The variable does not exist\n");
			}
		}
		else if(option==3){
			if(getenv(args[1])==NULL){
				printf("%s","No such variable exist\n");
			}
			else{
				printf("%s\n",getenv(args[1]));
			}
        }
	return 0;
}
 
void launchProg(char **args, int background){	 
	 if((pid=fork())==-1){
		 printf("Child process could not be created\n");
		 return;
	 }

	if(pid==0){	
		signal(SIGINT, SIG_IGN);
		setenv("parent",getcwd(PR_DIRECTORY, 1024),1);	
		
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
            ERROROR=execvp(args[0],args);
			printf("Command not found");
			kill(getpid(),SIGTERM);
        }
        return;
	 }
     
        // printf("%i",ERROROR);
		child_process[process_index]=pid;
		process_index++;
		// printf("%d %d",pid,process_index);
	 if (background == 0){
		 waitpid(pid,NULL,0);
	 }else{
		//  printf("Process created with PID: %d\n\n",pid);
	 }	 
}
 
void pipeHandler(char * args[]){
	// File descriptors
	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int ERROROR = -1;
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
	while (args[j] != NULL && end != 1){
		k = 0;
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
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

		child_process[process_index]=pid;
		process_index++;

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
			
			if (execvp(command[0],command)==ERROROR){
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
			
int cmd_runner(char * args[],int bg){
	int i = 0;
	int j = 0;
	int background = bg;

	char *args_aux[256];
	while ( args[j] != NULL){
		args_aux[j] = args[j];
		j++;
	}

    char *s;
	s=strchr(args[0],'=');
	if(s!=NULL){
		char *envi =   "setenv ";
		char *arguments= args[0];
		char command[MAX_CHARS];
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
		char * tokens[MAX_TOKENS]; 
		int numTokens;
		if((tokens[0] = strtok(command," \n\t")) == NULL) return 1;
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
		manageEnviron(tokens,1);
		return 1;
	}
	 
	if(strcmp(args[0],"echo") == 0 )	{
		char *s;
		s=strchr(args[1],'$');
		if(s!=NULL){
			char *envi =   "getenv ";
			char *arguments= args[1];
			char command[MAX_CHARS];
			int k=0;
			for(k=0;k<7;k++){
				command[k]=envi[k];
			}
			while(arguments[k-6]!=NULL){
				command[k]=arguments[k-6];
				k++;
			}
			char * tokens[MAX_TOKENS]; 
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
			printf("CommandNotFound\n");
			return 1;
		}
		launchProg(args,background);
	}
return 1;
}


/**
* Main method of our shell
*/ 
int main(int argc, char *argv[], char ** envp) {
	
	
	char line[MAX_CHARS]; 
	char * tokens[MAX_TOKENS]; 
	int numTokens;
	int bg=0;
	
	shell_print = 0; 	
	pid = -10; 
	init();
	environ = envp;
	setenv("shell",getcwd(PR_DIRECTORY, 1024),1);
	while(1){
		// int x=getpid();
		// printf("%ihduufh",x);
        ERROROR=0;
		if (shell_print == 0) shellPrompt();
		shell_print = 0;
		memset ( line, '\0', MAX_CHARS );
		fgets(line, MAX_CHARS, stdin);
		if(line[0]=='&'){
			bg=1;
		}
		if((tokens[0] = strtok(line," &\n\t")) == NULL) continue;
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " &\n\t")) != NULL) numTokens++;
        size_t size = sizeof(line) / sizeof(line[0]);
        cmd_history[h_index]=strndup(line, size);
        h_index++;
		cmd_runner(tokens,bg);
	}          

	exit(0);
}