#define _POSIX_C_SOURCE 200809L
/*Tarea Semana 4: Elena Perez Gonzalez-Tablas*/
/*[2022-06-07]*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_FILES 16

void primerHijo(int pipeMeMy[2], char *sizeChar, char *argvIn[], int num_files_in);
void segundoHijo(int pipeMeMy[2], int pipeMySp[2], char *sizeChar, char *lengthChar);
void tercerHijo(int pipeMySp[2], char *sizeChar, char *argvOut[], int num_files_out);
void mensaje_help(char *argv[]);
void cerrarDescriptor(int descriptor);

// ./merge_mystrings_split [-t BUFSIZE] [-n MINLENGTH] -i FILEIN1[, FILEIN2 ... FILEINn] FILEOUT1 [FILEOUT2 ... FILEOUTn]
int main(int argc, char *argv[])
{
    int opt, length = 4, size = 1024, num_files_in = 0, num_files_out = 0;
    char *files = NULL;
    char *sizeChar = NULL;
    char *lengthChar = NULL;
    char *argvIn[MAX_FILES];
    char *saveptr;
    char *argvOut[MAX_FILES];

    int pipeMeMy[2]; /* Descriptores de fichero de la tubería */
    int pipeMySp[2]; /* Descriptores de fichero de la tubería */

    optind = 1;
    // PROCESAMIENTO DE LA ENTRADA
    while ((opt = getopt(argc, argv, "t:n:i:h")) != -1)
    {
        switch (opt)
        {
        case 't':
            size = atoi(optarg);
            sizeChar = optarg;
            break;
        case 'n':
            length = atoi(optarg);
            lengthChar = optarg;
            break;
        case 'i':
            files = optarg;
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
    if (files != NULL)
    {
        argvIn[num_files_in] = strtok_r(files, ",", &saveptr);
        while (argvIn[num_files_in] != NULL)
        {
            if (num_files_in++ >= MAX_FILES)
            {
                fprintf(stderr, "Error: Demasiados ficheros de entrada. Máximo 16 ficheros.\n");
                mensaje_help(argv);
                exit(EXIT_FAILURE);
            }
            argvIn[num_files_in] = strtok_r(NULL, ",", &saveptr);
        }
        if (num_files_in == 0)
        {
            fprintf(stderr, "Error: No hay ficheros de entrada.\n");
            mensaje_help(argv);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "Error: Deben proporcionarse ficheros de entrada con la opción -i.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }

    num_files_out = argc - optind;
    if (num_files_out >= MAX_FILES)
    {
        fprintf(stderr, "Error: Demasiados ficheros de salida. Máximo 16 ficheros.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
    if (optind >= argc)
    {
        fprintf(stderr, "Error: Deben proporcionarse la lista de ficheros de salida.\n");
        mensaje_help(argv);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_files_out; i++)
    {
        argvOut[i] = argv[i + optind];
    }

    // CREACIÓN DE LA PRIMERA TUBERÍA
    if (pipe(pipeMeMy) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }
    // CREACIÓN PRIMER HIJO (merge)
    switch (fork())
    {
    case -1:
        perror("fork(1)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        primerHijo(pipeMeMy, sizeChar, argvIn, num_files_in);
        break;
    default: /* El proceso padre continúa... */
        break;
    }
    // CREACIÓN DE LA SEGUNDA TUBERÍA (la creo en este instante para que estos descriptores solo los tengan el padre, el proceso hijo mystrings y el proceso hijo split)
    if (pipe(pipeMySp) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }
    // CREACIÓN SEGUNDO HIJO (mytrings)
    switch (fork())
    {
    case -1:
        perror("fork(2)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        segundoHijo(pipeMeMy, pipeMySp, sizeChar, lengthChar);
        break;
    default: // El proceso padre continúa...
        break;
    }

    // EL PROCESO PADRE CIERRA LOS DESCRIPTORES DE FICHERO DE LA PRIMERA TUBERÍA (los cierro en este instante para que no los herede el hijo split puesto que no los necesita)
    cerrarDescriptor(pipeMeMy[0]);
    cerrarDescriptor(pipeMeMy[1]);

    // CREACIÓN TERCER HIJO (SPLIT)
    switch (fork())
    {
    case -1:
        perror("fork(2)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        tercerHijo(pipeMySp, sizeChar, argvOut, num_files_out);
        break;
    default: // El proceso padre continúa...
        break;
    }
    // EL PROCESO PADRE CIERRA LOS DESCRIPTORES DE FICHERO DE LA SEGUNDA TUBERÍA
    cerrarDescriptor(pipeMySp[0]);
    cerrarDescriptor(pipeMySp[1]);

    // EL PROCESO PADRE ESPERA A QUE TERMINEN SUS PROCESOS HIJOS
    for (int i = 0; i < 3; i++)
        if (wait(NULL) == -1)
        {
            perror("wait(1)");
            exit(EXIT_FAILURE);
        }

    exit(EXIT_SUCCESS);
}

void mensaje_help(char *argv[])
{
    fprintf(stderr, "Uso: %s [-t BUFSIZE] [-n MINLENGTH] -i FILEIN1[, FILEIN2, ..., FILEINn] FILEOUT1 [FILEOUT2 ... FILEOUTn]\n", argv[0]);
    fprintf(stderr, "Primero, fusiona los ficheros de entrada. Segundo, lee las cadenas compuestas por caracteres imprimibles incluyendo espacios, tabuladores y ");
    fprintf(stderr, "saltos de línea, que tengan una longitud mayor o igual a un tamaño dado.");
    fprintf(stderr, " Por último, divide en ficheros el flujo de bytes. \n");
    fprintf(stderr, "No admite lectura de la entrada estandar.\n");
    fprintf(stderr, "El número máximo de ficheros de salida y salida es 16.\n");
    fprintf(stderr, "-t BUFSIZE Tamaño de buffer donde MINLENGTH <= BUFSIZE <= 1MB (por defecto 1024).\n");
    fprintf(stderr, "-n MINLENGTH Longitud mínima de la cadena. Mayor que 0 y menor que 256 (por defecto 4).\n");
    fprintf(stderr, "-i FILEIN1, FILEIN2 ... FILEINn Lista de ficheros de entrada separados por comas.\n");
}

void cerrarDescriptor(int descriptor)
{ // FUNCIÓN AUXILIAR QUE CIERRA DESCRIPTORES
    if (close(descriptor) == -1)
    {
        perror("close()");
        exit(EXIT_FAILURE);
    }
}

void primerHijo(int pipeMeMy[2], char *sizeChar, char *argvIn[], int num_files_in)
{
    char *argvMerge[num_files_in + 4];
    int num = 0;
    /* Paso 2: El extremo de lectura no se usa */
    cerrarDescriptor(pipeMeMy[0]);
    /* Paso 3: Redirige la salida estándar al extremo de escritura de la tubería */
    if (dup2(pipeMeMy[1], STDOUT_FILENO) == -1)
    {
        perror("dup2(1)");
        exit(EXIT_FAILURE);
    }
    /* Paso 4: Cierra descriptor duplicado */
    cerrarDescriptor(pipeMeMy[1]);
    /* Paso 5: Reemplaza el binario actual por el de `merge_files` */

    argvMerge[num++] = "merge_files";

    if (sizeChar != NULL)
    {
        argvMerge[num++] = "-t";
        argvMerge[num++] = sizeChar;
    }

    for (size_t i = 0; i < num_files_in; i++)
    {
        argvMerge[i + num] = argvIn[i];
    }
    argvMerge[num_files_in + num] = NULL;
    execv("./merge_files", argvMerge);
    perror("execvp(merge_files)");
    exit(EXIT_FAILURE);
}

void segundoHijo(int pipeMeMy[2], int pipeMySp[2], char *sizeChar, char *lengthChar)
{
    char *argvMyStrings[6];
    int num = 0;
    /* Paso 7: El extremo de escritura no se usa */
    cerrarDescriptor(pipeMeMy[1]);
    /* Paso 8: Redirige la entrada estándar al extremo de lectura de la tubería */
    if (dup2(pipeMeMy[0], STDIN_FILENO) == -1)
    {
        perror("dup2(2)");
        exit(EXIT_FAILURE);
    }
    /* Paso 9: El extremo de escritura no se usa */
    cerrarDescriptor(pipeMeMy[0]);
    cerrarDescriptor(pipeMySp[0]);
    /* Paso 10: Redirige la salida estándar al extremo de escritura de la tubería */
    if (dup2(pipeMySp[1], STDOUT_FILENO) == -1)
    {
        perror("dup2(1)");
        exit(EXIT_FAILURE);
    }
    cerrarDescriptor(pipeMySp[1]);

    /* Paso 11: Reemplaza el binario actual por el de `mystrings` */
    argvMyStrings[num++] = "mystrings";
    if (sizeChar != NULL)
    {
        argvMyStrings[num++] = "-t";
        argvMyStrings[num++] = sizeChar;
    }
    if (lengthChar != NULL)
    {
        argvMyStrings[num++] = "-n";
        argvMyStrings[num++] = lengthChar;
    }
    argvMyStrings[num] = NULL;
    execv("./mystrings", argvMyStrings);
    perror("execv(mystrings)");
    exit(EXIT_FAILURE);
}

void tercerHijo(int pipeMySp[2], char *sizeChar, char *argvOut[], int num_files_out)
{
    char *argvSplit[num_files_out + 4];
    int num = 0;

    /* Paso 13: El extremo de escritura no se usa */
    cerrarDescriptor(pipeMySp[1]);
    /* Paso 14: Redirige la entrada estándar al extremo de lectura de la tubería */
    if (dup2(pipeMySp[0], STDIN_FILENO) == -1)
    {
        perror("dup2(2)");
        exit(EXIT_FAILURE);
    }
    /* Paso 15: El extremo de escritura no se usa */
    cerrarDescriptor(pipeMySp[0]);

    /* Paso 16: Reemplaza el binario actual por el de `split_files` */
    argvSplit[num++] = "split_files";

    if (sizeChar != NULL)
    {
        argvSplit[num++] = "-t";
        argvSplit[num++] = sizeChar;
    }

    for (size_t i = 0; i < num_files_out; i++)
    {
        argvSplit[i + num] = argvOut[i];
    }
    argvSplit[num_files_out + num] = NULL;

    execv("./split_files", argvSplit);
    perror("execvp(split_files)");
    exit(EXIT_FAILURE);
}
