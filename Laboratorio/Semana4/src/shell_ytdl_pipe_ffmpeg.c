#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

/*  Ejemplo: shell_ytdl_pipe_ffmeg -u YTURL -f FICHERO */

void print_help(char* program_name)
{
    fprintf(stderr, "Uso: %s -u YTURL -f FICHERO\n", program_name);
}

int main(int argc, char *argv[])
{
    int opt;
    char *url = NULL;
    char *filename = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "u:f:h")) != -1)
    {
        switch (opt)
        {
        case 'u':
            url = optarg;
            break;
        case 'f':
            filename = optarg;
            break;
        case 'h':
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (url == NULL || filename == NULL)
    {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    int pipefds[2]; /* Descriptores de fichero de la tubería */

    if (pipe(pipefds) == -1) /* Paso 0: Creación de la tubería */
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    /* Paso 1: Creación del hijo izquierdo de la tubería */
    switch (fork())
    {
    case -1:
        perror("fork(1)");
        exit(EXIT_FAILURE);
        break;
    case 0: /* Hijo izquierdo de la tubería */
        /* Paso 2: El extremo de lectura no se usa */
        if (close(pipefds[0]) == -1)
        {
            perror("close(1)");
            exit(EXIT_FAILURE);
        }
        /* Paso 3: Redirige la salida estándar al extremo de escritura de la tubería */
        if (dup2(pipefds[1], STDOUT_FILENO) == -1)
        {
            perror("dup2(1)");
            exit(EXIT_FAILURE);
        }
        /* Paso 4: Cierra descriptor duplicado */
        if (close(pipefds[1]) == -1)
        {
            perror("close(2)");
            exit(EXIT_FAILURE);
        }
        /* Paso 5: Reemplaza el binario actual por el de `youtube-dl` */
        execlp("youtube-dl", "youtube-dl", url, "-q", "-o", "-", NULL);
        perror("execlp(izquierdo)");
        exit(EXIT_FAILURE);
        break;
    default: /* El proceso padre continúa... */
        break;
    }

    /* Paso 6: Creación del hijo izquierdo de la tubería */
    switch (fork())
    {
    case -1:
        perror("fork(2)");
        exit(EXIT_FAILURE);
        break;
    case 0: /* Hijo derecho de la tubería  */
        /* Paso 7: El extremo de escritura no se usa */
        if (close(pipefds[1]) == -1)
        {
            perror("close(3)");
            exit(EXIT_FAILURE);
        }
        /* Paso 8: Redirige la entrada estándar al extremo de lectura de la tubería */
        if (dup2(pipefds[0], STDIN_FILENO) == -1)
        {
            perror("dup2(2)");
            exit(EXIT_FAILURE);
        }
        /* Paso 9: Cierra descriptor duplicado */
        if (close(pipefds[0]) == -1)
        {
            perror("close(4)");
            exit(EXIT_FAILURE);
        }
        /* Paso 10: Reemplaza el binario actual por el de `ffmpeg` */
        execlp("ffmpeg", "ffmpeg", "-i", "-", filename, NULL);
        perror("execlp(derecho)");
        exit(EXIT_FAILURE);
        break;
    default: /* El proceso padre continúa... */
        break;
    }

    /* El proceso padre cierra los descriptores de fichero no usados */
    if (close(pipefds[0]) == -1)
    {
        perror("close(pipefds[0])");
        exit(EXIT_FAILURE);
    }
    if (close(pipefds[1]) == -1)
    {
        perror("close(pipefds[1])");
        exit(EXIT_FAILURE);
    }
    /* El proceso padre espera a que terminen sus procesos hijo */
    if (wait(NULL) == -1)
    {
        perror("wait(1)");
        exit(EXIT_FAILURE);
    }
    if (wait(NULL) == -1)
    {
        perror("wait(2)");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
