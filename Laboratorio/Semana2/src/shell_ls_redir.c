#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid; /* Usado en el proceso padre para guardar el PID del proceso hijo */
    int fd;

    switch (pid = fork())
    {
    case -1: /* fork() falló */
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0:                             /* Ejecución del proceso hijo tras fork() con éxito */
        if (close(STDOUT_FILENO) == -1) /* Cierra la salida estándar */
        {
            perror("close()");
            exit(EXIT_FAILURE);
        }
        /* Abre el fichero "listado" al que se asigna el descriptor de fichero no usado más bajo, es decir, STDOUT_FILENO(1) */
        if ((fd = open("listado", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1)
        {
            perror("open()");
            exit(EXIT_FAILURE);
        }
        execlp("ls", "ls", "-la", NULL);      /* Sustituye el binario actual por /bin/ls */
        fprintf(stderr, "execlp() failed\n"); /* Esta línea no se debería ejecutar si la anterior tuvo éxito */
        exit(EXIT_FAILURE);
        break;
    default:                  /* Ejecución del proceso padre tras fork() con éxito */
        if (wait(NULL) == -1) /* Espera a que termine el proceso hijo */
        {
            perror("wait()");
            exit(EXIT_FAILURE);
        }
        break;
    }

    return EXIT_SUCCESS;
}
