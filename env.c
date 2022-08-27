#include <stdio.h>
#include <stdlib.h>
 #include <string.h>
 
int main(void)
{
   char *a="abc";
    int len = strlen(a);
    char buf[len+2];
    strcpy(buf, a);
    buf[len] = 'x';
    buf[len + 1] = 0;
   printf("%s",strdup(buf));
}
 