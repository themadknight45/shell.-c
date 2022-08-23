#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>

static volatile int keepRunning = 1;
void intHandler(int dummy) {
   keepRunning = 0 ;
}

void printDirectory()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s~$ ", cwd);
}

int main() {
	while(1){
	    printDirectory();
	    char *buffer=NULL;
	    size_t n;
	    signal(SIGINT, intHandler);
	    getline(&buffer,&n,stdin);
	}
	return 0;
}

