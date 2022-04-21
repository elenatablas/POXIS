#define _POSIX_C_SOURCE 200809L
/*Tarea Semana 3: Elena Pérez González-Tablas*/
/*[2021-10-10]*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#define MAX_FILES 16

void merge(int *fdin, int fdout, char **buf, int size, int num_files, int error_files);
void mensaje_help(char *argv[]);

// ./merge_files [-t BUFSIZE] [-o FILEOUT] FILEIN1 [FILEIN2 ... FILEINn]
int main(int argc, char *argv[])
{
    int opt, fdout, error_files = 0, size = 1024;
    int fdin[MAX_FILES];
    char *fileout = NULL;
    char **buf = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "t:o:h")) != -1)
    {
        switch (opt)
        {
        case 't':
            size = atoi(optarg);
            break;
        case 'o':
            fileout = optarg;
            break;
        case 'h':
        default:
            mensaje_help(argv);
            exit(EXIT_FAILURE);
        }
    }
    if (size < 1 || size > 134217728)
    {
        fprintf(stderr, "Error: Tamaño de buffer incorrecto.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
    /* Abre el fichero de salida */
    if (fileout != NULL)
    {
        fdout = open(fileout, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fdout == -1)
        {
            perror("open(fileout)");
            exit(EXIT_FAILURE);
        }
    }
    else /* Por defecto, la salida estándar */
        fdout = STDOUT_FILENO;

    int num_files = argc - optind;
    if (num_files >= MAX_FILES)
    {
        fprintf(stderr, "Error: Demasiados ficheros de entrada. Máximo 16 ficheros.\n"); 
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
    /* Abre cada fichero de entrada y lo escribe en 'fileout' */
    if (optind < argc)
    {
        if ((buf = malloc(num_files * sizeof(int))) == NULL)
        {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < num_files; i++)
        {
            fdin[i] = open(argv[i+optind], O_RDONLY);
            if (fdin[i] == -1)
            {
                perror("Error: No se puede abrir");
                error_files++;
            }  
            if ((buf[i] = (char *)malloc(size * sizeof(char))) == NULL)
            {
                perror("malloc()");
                exit(EXIT_FAILURE);
            }
        }
        merge(fdin, fdout, buf, size, num_files, error_files);
    }
    else
    {
        fprintf(stderr, "Error: No hay ficheros de entrada.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
        
    if (close(fdout) == -1)
    {
        perror("close(fdout)");
        exit(EXIT_FAILURE);
    }

    /*for (int i = 0; i < num_files; i++)
        free(buf[i]);*/
    free(buf);

    exit(EXIT_SUCCESS);
}

void mensaje_help(char *argv[])
{
    fprintf(stderr, "Uso: %s [-t BUFSIZE] [-o FILEOUT] FILEIN1 [FILEIN2 ... FILEINn]\n", argv[0]);
    fprintf(stderr, "No admite lectura de la entrada estandar.\n");
    fprintf(stderr, "-t BUFSIZE Tamaño de buffer donde 1 <= BUFSIZE <= 128MB.\n");
    fprintf(stderr, "-o FILEOUT Usa FILEOUT en lugar de la salida estandar.\n");     
}

char * merge_part(char **buf, int num_files, int *num_read, int read_total, char *buf_salida)
{
    int posicion = 0;
    int *num = num_read;
    char * buffer = buf_salida;
    for(int j = 0; posicion < read_total; j++)
        for (int i = 0; i < num_files; i++){
            if(num[i] != 0)
            {
                num[i]--;
                buffer[posicion++] = buf[i][j];
            }
        }
    return buffer;
}

int write_all(int fd, char *buf, int read_total){
/* Escrituras parciales tratadas */
    ssize_t num_written;
    ssize_t num_salida = read_total;
    char * buf_salida = buf;

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

void merge(int *fdin, int fdout, char **buf, int size, int num_files, int error_files)
{
    ssize_t num_written = -1;
    int read_total;
    int * num_read = NULL;
    char * buf_salida = NULL;
    int read_files = num_files-error_files;
    if ((num_read = malloc(read_files * sizeof(int))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    while(read_files != 0)
    {
        read_total = 0;
        for (int i = 0; i < num_files; i++)
        {
            if(fdin[i] != -1)
            { 
                if ((num_read[i] = read(fdin[i], buf[i], size)) > 0)
                {
                    read_total += num_read[i];
                }
                else
                {
                    read_files--;
                    if (num_read[i] == -1)
                    {
                        perror("read(fdin)");
                        exit(EXIT_FAILURE);
                    }
                    if (close(fdin[i]) == -1)
                    {
                        perror("close(fdin)");
                        exit(EXIT_FAILURE);
                    }
                    fdin[i]= -1;
                }
            } 
        }
        if ((buf_salida = (char *)malloc(read_total * num_files * sizeof(char))) == NULL)
        {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        merge_part(buf, num_files, num_read, read_total, buf_salida);
        num_written = write_all(fdout, buf_salida, read_total);
        free(buf_salida);
        if (num_written == -1)
        {
            perror("write(fdin)");
            exit(EXIT_FAILURE);
        } 
    }
    free(num_read);
}
