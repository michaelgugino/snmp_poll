#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  char *name;
  char *community;
} hosttype;

int main(void)
{
    hosttype host;
    int i;
    int c = 0;
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("my6", "r");
    if (fp == NULL)
        exit(1);
    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu :\n", read);
        printf("%s", line);
        i = strlen(line)-1;
        if( line[ i ] == '\n')
          line[i] = '\0';
        }
        host[0]->name = line;
    if (ferror(fp)) {
        /* handle error */
    }
    free(line);
    fclose(fp);
    return 0;
}
