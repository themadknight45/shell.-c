#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
 
int main(void)
{
   char line[200]; 
   char * tokens[256]; 
        memset ( line, '\0', 200 );
		fgets(line, 200, stdin);
		if(line[0]=='\0')return 1;
		int index=0;int indexl=0;
        printf("%s",line);
        size_t size = strlen(line);
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
			index++;
			if(u==size){
				tokens[index]=NULL;break;
			}
			indexl=u+1;
		}
}