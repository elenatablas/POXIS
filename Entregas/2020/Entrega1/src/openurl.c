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

void hijos(pid_t pid, char *n, char *url);
void padre(pid_t pid, char *url, char *binario);

/* TAREA->./openurl [-n NAVEGADOR] [URLs] */
/* VERIFICACIÓN->./openurl [-n evince] [PDFs] */
int main(int argc, char **argv)
{
    pid_t pid[argc - optind];
    int opt;
    char *n = NULL;
    optind = 1;

    /* 1- Si no se especifica -n NAVEGADOR. */
    if (argc < 3)
    {
        fprintf(stderr, "Uso: %s [-n NAVEGADOR] [URLs]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Procesamiento de la entrada */
    while ((opt = getopt(argc, argv, "n:")) != ERROR)
    {
        switch (opt)
        {
        case 'n':
            n = optarg;
            break;
        case 'h': // Help
        default:  // Error
            fprintf(stderr, "Uso: %s [-n NAVEGADOR] [URLs]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* CASO PARTICULAR-> ./openurl [-n NAVEGADOR]*/
    if (optind == argc)
    {
        /* 3.2- Crear un proceso hijo con la aplicación externa NAVEGADOR para abrir la dirección www.um.es. */
        pid_t umu = fork();
        hijos(umu, n, "www.um.es");

        padre(umu, "www.um.es", argv[0]);

        /*VERIFICACIÓN*/
        //hijos(umu, n, "fichero1.pdf");
        //padre(umu, "fichero1.pdf", argv[0]);
    }
    else
    { /* CASO GENERAL-> ./openurl [-n NAVEGADOR] [URLs]*/
        /* 3.1- Crear un proceso hijo con la aplicación externa NAVEGADOR para abrir cada URL. */
        for (int i = 0; i < (argc - optind); i++)
        {
            pid[i] = fork();
            hijos(pid[i], n, argv[i + optind]);
            //sleep(1);
        }

        /* 4- El proceso padre debe esperar a que concluyan todos ellos en orden de creación. */
        for (int i = 0; i < argc - optind; i++)
        {
            padre(pid[i], argv[i + optind], argv[0]);
        }
    }
    return EXIT_SUCCESS;
}

void hijos(pid_t pid, char *n, char *url)
{
    /* fork() falló */
    if (pid == ERROR)
    {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    /* Ejecución del proceso hijo tras fork() con éxito */
    if (pid == 0)
    {
        execlp(n, n, "--new-window", url, NULL);

        /*VERIFICACIÓN*/
        //execlp(n, n, url, NULL);

        /* 2- Si el binario NAVEGADOR no se encuentra, el proceso hijo devolverá un mensaje de error. */
        fprintf(stderr, "Error: NAVEGADOR no encontrado\n");
        exit(EXIT_FAILURE);
    }
}

void padre(pid_t pid, char *url, char *binario)
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
        printf("%s: URL: %s, STATUS=%d\n", binario, url, WEXITSTATUS(status));

    else if (WIFSIGNALED(status))
        printf("%s: URL: %s, STATUS=%d\n", binario, url, WTERMSIG(status));
}