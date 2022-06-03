#define _POSIX_C_SOURCE 200809L
/*Tarea Semana 4: Elena Pérez González-Tablas*/
/*[2022-06-03]*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

void mystrings(int fdin, int fdout, char *buf, int size, int length);
void mensaje_help(char *argv[]);

// ./mystrings [-t BUFSIZE] [-n MINLENGTH]
int main(int argc, char *argv[])
{
    int opt, fdout, length = 4, size = 1024;

    char *buf = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "t:n:h")) != -1)
    {
        switch (opt)
        {
        case 't':
            size = atoi(optarg);
            break;
        case 'n':
            length = atoi(optarg);
            break;
        case 'h':
        default:
            mensaje_help(argv);
            exit(EXIT_FAILURE);
        }
    }
    if (length < 1 || length > 255)
    {
        fprintf(stderr, "Error: Número mínimo de caracteres imprimibles incorrecto.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
    if (size < length || size > 1048576)
    {
        fprintf(stderr, "Error: Tamaño de buffer incorrecto.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }

    /* Reserva memoria dinámica para buffer de lectura */
    if ((buf = (char *)malloc(size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    /* Salida estándar */
    fdout = STDOUT_FILENO;

    mystrings(STDIN_FILENO, fdout, buf, size, length);

    if (close(fdout) == -1)
    {
        perror("close(fdout)");
        exit(EXIT_FAILURE);
    }

    /* Libera memoria dinámica de buffer de lectura */
    free(buf);

    exit(EXIT_SUCCESS);
}

void mensaje_help(char *argv[])
{
    fprintf(stderr, "Uso: %s [-t BUFSIZE] [-n MINLENGTH]\n", argv[0]);
    fprintf(stderr, "Lee de la entrada estándar el flujo de bytes recibido y escribe en la salida estándar ");
    fprintf(stderr, "las cadenas compuestas por caracteres imprimibles incluyendo espacios, tabuladores y ");
    fprintf(stderr, "saltos de línea, que tengan una longitud mayor o igual a un tamaño dado.\n");
    fprintf(stderr, "-t BUFSIZE Tamaño de buffer donde MINLENGTH <= BUFSIZE <= 1MB (por defecto 1024).\n");
    fprintf(stderr, "-n MINLENGTH Longitud mínima de la cadena. Mayor que 0 y menor que 256 (por defecto 4).\n");
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
        perror("write(fdin)");
        exit(EXIT_FAILURE);
    }
    return num_salida;
}

void mystrings(int fdin, int fdout, char *buf, int size, int length)
{
    ssize_t num_read, num_written;
    char *buf_salida = NULL;
    char *buf_aux = NULL;
    int contador = -1, posicion = 0;
    bool candidato = false;

    /* Reserva memoria dinámica para buffer de lectura */
    if ((buf_salida = (char *)malloc(size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    if ((buf_aux = (char *)malloc(length * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    while ((num_read = read_all(fdin, buf, size)) > 0)
    {
        for (int i = 0; i < num_read; i++)
        {
            if(isprint(buf[i])){
                if(candidato){
                    buf_salida[posicion++] = buf[i];
                    if(posicion == size){
                        num_written = write_all(fdout, buf_salida, size);
                        if (num_written == -1)
                        {
                            perror("write(fdin)");
                            exit(EXIT_FAILURE);
                        } 
                        posicion = 0;
                    }
                }
                else{
                    buf_aux[contador++] = buf[i];
                    if(contador == length){
                        candidato = true;
                        for (int j = 0; j < length; j++){
                            buf_salida[posicion++] = buf_aux[j];
                            if(posicion == size){
                                num_written = write_all(fdout, buf_salida, size);
                                if (num_written == -1)
                                {
                                    perror("write(fdin)");
                                    exit(EXIT_FAILURE);
                                } 
                                posicion = 0;
                            }
                        }
                    }
                }
            }
            else{
                candidato = false;
                contador = 0;
            }
        }
    }
    num_written = write_all(fdout, buf_salida, posicion);
    if (num_written == -1)
    {
        perror("write(fdin)");
        exit(EXIT_FAILURE);
    }

    if (num_read == -1)
    {
        perror("read(fdin)");
        exit(EXIT_FAILURE);
    }

    if (close(fdin) == -1)
    {
        perror("close(fdin)");
        exit(EXIT_FAILURE);
    }
    free(buf_aux);
    free(buf_salida);
}
