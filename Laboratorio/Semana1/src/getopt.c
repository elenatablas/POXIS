#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int opt, flag = 0, n = 0;
    char *s = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "fn:s:h")) != -1)
    {
        switch (opt)
        {
        case 'f':
            flag = 1;
            break;
        case 'n':
            n = atoi(optarg);
            break;
        case 's':
            s = optarg;
            break;
        case 'h':
        default:
            fprintf(stderr, "Uso: %s [-f] [-n NUMERO] [-s STRING] [PARAMS]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("flag: %d, n: %d, s: \"%s\"", flag, n, s);
    for (int i = optind; i < argc; i++)
        printf(", param%d: \"%s\"", i - optind, argv[i]);
    printf("\n");

    return EXIT_SUCCESS;
}