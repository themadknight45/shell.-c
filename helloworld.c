#include <stdio.h>
#include <string.h>
char testCmd[100];
char usrCmd[100];
int main() {
	strcpy(testCmd, "which ");
	strcpy(usrCmd, "ls");
	strncat(testCmd, usrCmd, 93);
	size_t size = 0;
	char path[100];
	FILE *p = popen( testCmd , "r");
	if(p ) {
		while(fgets(path, sizeof(path), p) != NULL) {
					size= strlen(path);
		}
	}
	if(size==0){
		printf("No Cmd ");
	}
}

