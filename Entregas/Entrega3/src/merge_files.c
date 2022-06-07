#define _POSIX_C_SOURCE 200809L
/*Tarea Semana 3: Elena Pérez González-Tablas*/
/*[2022-06-03]*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#define MAX_FILES 16

void merge(int *fdin, int fdout, char *buf, int size, int num_files, int error_files);
void mensaje_help(char *argv[]);

// ./merge_files [-t BUFSIZE] [-o FILEOUT] FILEIN1 [FILEIN2 ... FILEINn]
int main(int argc, char *argv[])
{
    int opt, fdout, error_files = 0, size = 1024;
    int fdin[MAX_FILES];
    char *fileout = NULL;
    char *buf = NULL;

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
        if ((buf = (char *)malloc(size * sizeof(char))) == NULL)
        {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < num_files; i++)
        {
            fdin[i] = open(argv[i+optind], O_RDONLY);
            if (fdin[i] == -1)
            {
                fprintf(stderr,"Error: El fichero '%s' no existe o no tiene permisos de lectura.\n",argv[i+optind]);
                error_files++;
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

void merge_part(char *buf, int read_files, int *num_read, int size_read_file, char *buf_salida)
{
    int posicion = 0;
    int *num = num_read;
    char * buffer = buf_salida;
    for(int j = 0; j < size_read_file; j++)
        for (int i = 0; i < read_files; i++){
            if(num[i] != 0)
            {
                num[i]--;
                buffer[posicion++] = buf[i*size_read_file+j];
            }
        }
}

int read_all(int fd, char *buf, int size_read){
/* Lecturas parciales tratadas */
    ssize_t num_read;
    ssize_t num_entrada = size_read;
    char * buf_entrada = buf;

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
    return size_read-num_entrada;
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



void merge(int *fdin, int fdout, char *buf_entrada, int size, int num_files, int error_files)
{
    ssize_t num_written = -1;
    int read_total, read_files, size_read_file;
    int unread_files = num_files-error_files;

    int * num_read = NULL;
    char * buf_salida = NULL;
    
    if ((num_read = malloc(unread_files * sizeof(int))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    if ((buf_salida = (char *)malloc(size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    while(unread_files != 0)
    {
        read_total = 0;
        read_files = 0;
        size_read_file = size/unread_files+1;
        for (int i = 0; i < num_files; i++)
        {
            if(fdin[i] != -1)
            {
                if ((num_read[read_files] = read_all(fdin[i], buf_entrada+(read_files*size_read_file), size_read_file)) > 0)
                {
                    read_total += num_read[read_files];
                    read_files++;
                }
                else
                {
                    unread_files--;
                    if (num_read[read_files] == -1)
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
        merge_part(buf_entrada, read_files, num_read, size_read_file, buf_salida);
        num_written = write_all(fdout, buf_salida, read_total);
        if (num_written == -1)
        {
            perror("write(fdin)");
            exit(EXIT_FAILURE);
        } 
    }
    free(buf_salida);
    free(num_read);
}
