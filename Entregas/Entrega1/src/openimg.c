#define _POSIX_C_SOURCE 200809L
/* Tarea Semana 2: Elena Perez Gonzalez-Tablas*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ERROR -1

void hijos(char *v, char *img);
void padre(pid_t pid, char *img, char *binario);

/* TAREA->./openimg [-v VISOR] [IMGs] */
/* VERIFICACIÓN->./openimg [-v inkscape] [IMGs] */ // fbi o feh probar
int main(int argc, char **argv)
{
    pid_t pid[argc - optind];
    int opt;
    char *v = NULL;
    optind = 1;

    /* 1- Si no se especifica -v VISOR. */
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s [-v VISOR] [IMGs]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Procesamiento de la entrada */
    while ((opt = getopt(argc, argv, "v:")) != ERROR)
    {
        switch (opt)
        {
        case 'v':
            v = optarg;
            break;
        case 'h': // Help
        default:  // Error
            fprintf(stderr, "Uso: %s [-v VISOR] [IMGs]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* 2- CASO PARTICULAR-> ./openimg [-v VISOR]*/
    if (optind == argc)
    {
        fprintf(stderr, "Error: No hay imágenes que visualizar\n");
        exit(EXIT_FAILURE);
    }
    else
    { /* CASO GENERAL-> ./openimg [-v VISOR] [IMGs]*/
        /* 3- Crear un proceso hijo con la aplicación externa VISOR para abrir cada IMG. */
        for (int i = 0; i < (argc - optind); i++)
        {
            pid[i] = fork();
            /* fork() falló */
            if (pid[i] == ERROR)
            {
                perror("fork()");
                exit(EXIT_FAILURE);
            }
            if (pid[i] == 0)
            {
                hijos(v, argv[i + optind]);
            }
        }

        /* 4- El proceso padre debe esperar a que concluyan todos ellos en orden de creación. */
        for (int i = 0; i < argc - optind; i++)
        {
            padre(pid[i], argv[i + optind], argv[0]);
        }
    }
    return EXIT_SUCCESS;
}

void hijos(char *v, char *img)
{
    /* Ejecución del proceso hijo tras fork() con éxito */
    execlp(v, v, img, NULL);

    /* 2- Si el binario VISOR no se encuentra, el proceso hijo devolverá un mensaje de error. */
    fprintf(stderr, "Error: VISOR no encontrado\n");
    exit(EXIT_FAILURE);

}

void padre(pid_t pid, char *img, char *binario)
{
    /* Ejecución del proceso padre tras fork() con éxito */
    int status;

    if (waitpid(pid, &status, 0) == ERROR) // El padre espera a que acabe el hijo
    {
        perror("waitpid()");
        exit(EXIT_FAILURE);
    }

    /* 5- Cada vez que termine un proceso hijo, el proceso padre debe mostrar un mensaje.*/
    if (WIFEXITED(status))
        printf("%s: IMG: %s, STATUS=%d\n", binario, img, WEXITSTATUS(status));

    else if (WIFSIGNALED(status))
        printf("%s: IMG: %s, STATUS=%d\n", binario, img, WTERMSIG(status));
}