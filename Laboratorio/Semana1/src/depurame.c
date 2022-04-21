#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **inicializa()
{
    char **tmp;
    int i;

    tmp = malloc(2000 * sizeof(char *));
    for (i = 0; i < 20000; i++)
        tmp[i] = 0;

    return tmp;
}

void copia(char **buffers)
{
    strcpy(buffers[0], "ASO");
}

int main(void)
{
    char **buffers;

    buffers = inicializa();
    copia(buffers);
    printf("%s\n", buffers[0]);

    return EXIT_SUCCESS;
}