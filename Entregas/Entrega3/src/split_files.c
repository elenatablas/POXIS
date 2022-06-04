#define _POSIX_C_SOURCE 200809L
/*Tarea Semana 4: Elena Pérez González-Tablas*/
/*[2022-06-04]*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#define MAX_FILES 16

void split(int fdin, int *fdout, char *buf, int size, int num_files);
void mensaje_help(char *argv[]);

// ./split_files [-t BUFSIZE] FILEOUT1 [FILEOUT2 ... FILEOUTn]
int main(int argc, char *argv[])
{
    int opt, error_files = 0, size = 1024;
    int fdout[MAX_FILES];
    char *buf = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "t:h")) != -1)
    {
        switch (opt)
        {
        case 't':
            size = atoi(optarg);
            break;
        case 'h':
        default:
            mensaje_help(argv);
            exit(EXIT_FAILURE);
        }
    }
    if (size < 1 || size > 1048576)
    {
        fprintf(stderr, "Error: Tamaño de buffer incorrecto.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }

    int num_files = argc - optind;
    if (num_files >= MAX_FILES)
    {
        fprintf(stderr, "Error: Demasiados ficheros de salida. Máximo 16 ficheros.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }

    /* Abre cada fichero de salida */
    if (optind < argc)
    {
        if ((buf = (char *)malloc(size * sizeof(char))) == NULL)
        {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < num_files; i++)
        {
            fdout[i] = open(argv[i + optind], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fdout[i] == -1)
            {
                perror("Error: No se puede abrir/crear");
                exit(EXIT_FAILURE);
            }
        }
        split(STDIN_FILENO, fdout, buf, size, num_files);
    }
    else
    {
        fprintf(stderr, "Error: No hay ficheros de salida.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_files; i++)
    {
        if (close(fdout[i]) == -1)
        {
            perror("close(fdout)");
            exit(EXIT_FAILURE);
        }
    }

    free(buf);

    exit(EXIT_SUCCESS);
}

void mensaje_help(char *argv[])
{
    fprintf(stderr, "Uso: %s [-t BUFSIZE] FILEOUT1 [FILEOUT2 ... FILEOUTn]\n", argv[0]);
    fprintf(stderr, "Divide en ficheros el flujo de bytes que recibe por la entrada estandar. El número máximo de ficheros de salida es 16.\n");
    fprintf(stderr, "-t BUFSIZE Tamaño de buffer donde 1 <= BUFSIZE <= 1MB (por defecto 1024).\n");
}

int read_all(int fd, char *buf, int size_read)
{
    /* Lecturas parciales tratadas */
    ssize_t num_read;
    ssize_t num_entrada = size_read;
    char *buf_entrada = buf;

    while (num_entrada > 0 && (num_read = read(fd, buf_entrada, num_entrada)) > 0)
    {
        buf_entrada += num_read;
        num_entrada -= num_read;
    }
    if (num_read == -1)
    {
        perror("read(fdin)");
        exit(EXIT_FAILURE);
    }
    return size_read - num_entrada;
}

int write_all(int fd, char *buf, int read_total)
{
    /* Escrituras parciales tratadas */
    ssize_t num_written;
    ssize_t num_salida = read_total;
    char *buf_salida = buf;

    while (num_salida > 0 && (num_written = write(fd, buf_salida, num_salida)) != -1)
    {
        buf_salida += num_written;
        num_salida -= num_written;
    }
    if (num_written == -1)
    {
        perror("write(fdout)");
        exit(EXIT_FAILURE);
    }
    return num_salida;
}

int split_part(char *buf_entrada, char **buf_salida, int num_files, int num_read, int size_write_file, int fichero)
{
    int posicion = 0;
    for (int j = 0; j < num_read/num_files+1; j++){
        for (int i = 0; posicion < num_read && i < num_files; i++){
            buf_salida[fichero][j] = buf_entrada[posicion++];
            printf("(%d)Fichero f%d \n", j, fichero);
            fichero = (fichero + 1) % num_files;
        }
    }
    return fichero;
}

void split(int fdin, int *fdout, char *buf_entrada, int size, int num_files)
{
    ssize_t num_written = -1, num_read = -1;

    int act_fichero = 0, fichero = 0;
    int sig_fichero = 0;

    int size_write_file = size / num_files + 1;

    char **buf_salida = NULL;
    if ((buf_salida = malloc(num_files * sizeof(int))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_files; i++)
    {
        if ((buf_salida[i] = (char *)malloc(size * sizeof(char))) == NULL)
        {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
    }

    while ((num_read = read_all(fdin, buf_entrada, size)) > 0)
    {
        size_write_file = num_read / num_files+1;
        printf("size_write_file = %d \n", size_write_file);
        fichero = sig_fichero;
        sig_fichero = split_part(buf_entrada, buf_salida, num_files, num_read, size_write_file, fichero);
        for (int i = 0; i < size && i < num_files; i++)
        {
            act_fichero = (fichero+i)%num_files;
            printf("Fichero escrito f%d \n", act_fichero);
            printf("Contenido %s \n",buf_salida[act_fichero]);
            num_written = write_all(fdout[act_fichero], buf_salida[act_fichero], size_write_file);
            if (num_written == -1)
            {
                perror("write(fdout)");
                exit(EXIT_FAILURE);
            }
            
        }
    }

    free(buf_salida);
    if (close(fdin) == -1)
    {
        perror("close(fdin)");
        exit(EXIT_FAILURE);
    }
}
