#define _POSIX_C_SOURCE 200809L
/*Tarea Semana 4: Elena Perez Gonzalez-Tablas*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

//El tiempo de ejecución de este programa con la entrada "base64 /dev/urandom | head -c 1000000000 > entrada ; "
//"$(which time) -f "%e segundos" ./cat_pipe_tr_pipe_wc -s a -d A entrada;" ronda los 2.93 segundos con los tamaños de buffers a 4096
// con respecto al tamaño 1024 varía 1.43 segundos y con el tamaño 8192 solo disminuye 0.19 segundos. Por tanto, mirando la gráfica 
// he decidido que este es el tamaño idóneo para mi código. La diferencia de tiempo con distintos tamaños para cada buffer es despreciable
// por eso todos tienen el mismo.
#define BUF_CAT 4096
#define BUF_TR 4096
#define BUF_WC 4096

char *procesarCaracterControl(char *entrada);
void cerrarDescriptor(int descriptor);
void primerHijo(int pipefds[2], int argc, char *argv[]);
void segundoHijo(int pipefds[2], int pipeTrWc[2], char *carAscii);
void tercerHijo(int pipeTrWc[2]);

int main(int argc, char *argv[])
{
    int opt;
    char *src = NULL;
    char *dst = NULL;
    char carAscii[256];
    // INICIALIZO EL ARRAY DE CARACTERES ASCII EXTENDIDO
    for (int i = 0; i < 256; i++)
        carAscii[i]=i;

    // VARIABLE PARA SABER SI HAY QUE PROCESAR LOS CARACTERES DE CONTROL
    bool recControl = false;
    int pipeCatTr[2]; /* Descriptores de fichero de la tubería */
    int pipeTrWc[2];  /* Descriptores de fichero de la tubería */

    optind = 1;

    if (argc < 5)
    {
        fprintf(stderr, "Uso: %s [-e] -s SRC -d DST [FILEIN1 FILEIN2 ... FILEINn]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // PROCESAMIENTO DE LA ENTRADA
    while ((opt = getopt(argc, argv, "s:d:eh")) != -1)
    {
        switch (opt)
        {
        case 's':
            src = optarg;
            break;
        case 'd':
            dst = optarg;
            break;
        case 'e':
            recControl = true;
            break;
        case 'h':
        default:
            fprintf(stderr, "Uso: %s [-e] -s SRC -d DST [FILEIN1 FILEIN2 ... FILEINn]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    // PROCESAMIENTO CUANDO LAS CADENAS SRC Y DST ADMITEN CARACTERES DE CONTROL
    if (recControl == true)
    {
        src = procesarCaracterControl(src);
        dst = procesarCaracterControl(dst);
    }
    // LAS CADENAS DEBEN DE SER NO NULAS Y TENER LA MISMA LONGITUD
    if ((strlen(src) != strlen(dst)) || strlen(dst) == 0)
    {
        fprintf(stderr, "Error: SRC y DST deben ser cadenas de caracteres de la misma longitud\n");
        exit(EXIT_FAILURE);
    }
    // AÑADIR A LA TABLA LOS CARACTERES QUE SE CAMBIAN DE VALOR
    for (int i = 0; i < strlen(src); i++)
        carAscii[(int)src[i]] = dst[i];

    // CREACIÓN DE LA PRIMERA TUBERÍA
    if (pipe(pipeCatTr) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }
    // CREACIÓN PRIMER HIJO (CAT)
    switch (fork())
    {
    case -1:
        perror("fork(1)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        primerHijo(pipeCatTr, argc, argv);
        break;
    default: /* El proceso padre continúa... */
        break;
    }
    // CREACIÓN DE LA SEGUNDA TUBERÍA (la creo en este instante para que estos descriptores solo los tengan el padre, el proceso hijo tr y el proceso hijo de wc)
    if (pipe(pipeTrWc) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }
    // CREACIÓN SEGUNDO HIJO (TR)
    switch (fork())
    {
    case -1:
        perror("fork(2)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        segundoHijo(pipeCatTr, pipeTrWc, carAscii);
        break;
    default: // El proceso padre continúa...
        break;
    }

    // EL PROCESO PADRE CIERRA LOS DESCRIPTORES DE FICHERO DE LA PRIMERA TUBERÍA (los cierro en este instante para que no los herede el hijo wc puesto que no los necesita)
    cerrarDescriptor(pipeCatTr[0]);
    cerrarDescriptor(pipeCatTr[1]);

    // CREACIÓN TERCER HIJO (WC)
    switch (fork())
    {
    case -1:
        perror("fork(2)");
        exit(EXIT_FAILURE);
        break;
    case 0:
        tercerHijo(pipeTrWc);
        break;
    default: // El proceso padre continúa...
        break;
    }
    // EL PROCESO PADRE CIERRA LOS DESCRIPTORES DE FICHERO DE LA SEGUNDA TUBERÍA
    cerrarDescriptor(pipeTrWc[0]);
    cerrarDescriptor(pipeTrWc[1]);

    // EL PROCESO PADRE ESPERA A QUE TERMINEN SUS PROCESOS HIJOS
    for (int i = 0; i < 3; i++)
        if (wait(NULL) == -1)
        {
            perror("wait(1)");
            exit(EXIT_FAILURE);
        }
    // LIBERAMOS MEMORIA SI ESTABA EL PARÁMETRO -e EN LA ENTRADA
    if (recControl == true)
    {
        free(src);
        free(dst);
    }
    exit(EXIT_SUCCESS);
}

char *reservarBuffer(unsigned tamagno)
{ // FUNCIÓN AUXILIAR PARA RESERVAR MEMORIA
    char *buffer;
    if ((buffer = (char *)malloc(tamagno * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    return buffer;
}

char *procesarCaracterControl(char *entrada)
{ // DADA UNA CADENA DEVUELVE OTRA IGUAL PERO MODIFICANDO LOS CARACTERES CONSECUTIVOS "\n" POR EL CARACTER '\n'
    char *aux = reservarBuffer(strlen(entrada));
    int contador = 0;
    for (int i = 0; i < strlen(entrada); i++)
    {
        if (entrada[i] == '\\' && i + 1 < strlen(entrada) && entrada[i + 1] == 'n')
        {
            aux[contador++] = '\n';
            i++;
        }
        else
        {
            aux[contador++] = entrada[i];
        }
    }
    return aux;
}

void cerrarDescriptor(int descriptor)
{ // FUNCIÓN AUXILIAR QUE CIERRA DESCRIPTORES
    if (close(descriptor) == -1)
    {
        perror("close()");
        exit(EXIT_FAILURE);
    }
}

int write_all(int fd, char *buf, unsigned size)
{ // FUNCIÓN PARA PROCESAR LAS ESCRITURAS PARCIALES
    ssize_t num_written;
    ssize_t num_left;
    char *buf_left;

    num_left = size;
    buf_left = buf;
    while (num_left > 0 && (num_written = write(fd, buf_left, num_left)) != -1)
    {
        buf_left += num_written;
        num_left -= num_written;
    }
    if (num_written == -1)
    {
        perror("write(fdin)");
        exit(EXIT_FAILURE);
    }
    return size;
}

void catfd(int fdin, int fdout, char *buf, unsigned buf_size)
{ // PRIMER COMANDO
    ssize_t num_read, num_written;

    while ((num_read = read(fdin, buf, buf_size)) > 0)
    {
        num_written = write_all(fdout, buf, num_read);
        if (num_written == -1)
        {
            perror("write(fdin)");
            exit(EXIT_FAILURE);
        }
    }
    if (num_read == -1)
    {
        perror("read(fdin)");
        exit(EXIT_FAILURE);
    }
    cerrarDescriptor(fdin);
}

void primerHijo(int pipeCatTr[2], int argc, char *argv[])
{ // CIERRA LOS DESCRIPTORES QUE NO NECESITA, LLAMA A catfd, ENVÍA SU SALIDA A LA TUBERÍA Y ACABA LA EJECUCIÓN DE ESTE PROCESO
    cerrarDescriptor(pipeCatTr[0]);

    char *bufCat = reservarBuffer(BUF_CAT);

    // ABRE CADA FICHERO DE ENTRADA Y LO ESCRIBE EN 'pipeCatTr[0]'
    if (optind < argc)
        for (int i = optind; i < argc; i++)
        {
            int fdin = open(argv[i], O_RDONLY);
            if (fdin == -1)
            {
                perror("open(filein)");
                continue;
            }
            catfd(fdin, pipeCatTr[1], bufCat, BUF_CAT);
        }
    else
        catfd(STDIN_FILENO, pipeCatTr[1], bufCat, BUF_CAT);

    free(bufCat);
    cerrarDescriptor(pipeCatTr[1]);
    exit(EXIT_SUCCESS);
}

void trfd(int fdin, int fdout, char *buf, char *carAscii, unsigned buf_size)
{// SEGUNDO COMANDO
    ssize_t num_read, num_written;

    while ((num_read = read(fdin, buf, buf_size)) > 0)
    {
        for(int i=0; i<buf_size; i++)
            buf[i] = carAscii[(int)buf[i]];
        num_written = write_all(fdout, buf, num_read);
        if (num_written == -1)
        {
            perror("write(fdin)");
            exit(EXIT_FAILURE);
        }
    }
    if (num_read == -1)
    {
        perror("read(fdin)");
        exit(EXIT_FAILURE);
    }
    cerrarDescriptor(fdin);
}

void segundoHijo(int pipeCatTr[2], int pipeTrWc[2], char *carAscii)
{ // CIERRA LOS DESCRIPTORES QUE NO NECESITA, LLAMA A trfd, ENVÍA SU SALIDA A LA TUBERÍA Y ACABA LA EJECUCIÓN DE ESTE PROCESO
    cerrarDescriptor(pipeCatTr[1]);
    cerrarDescriptor(pipeTrWc[0]);

    char *bufTr = reservarBuffer(BUF_TR);

    trfd(pipeCatTr[0], pipeTrWc[1], bufTr, carAscii, BUF_TR);

    free(bufTr);
    cerrarDescriptor(pipeTrWc[1]);
    exit(EXIT_SUCCESS);
}

long contarLineas(char *buf, unsigned num_read)
{ // FUNCIÓN AUXILIAR DE wcfd PARA CONTAR EL NUMERO DE LÍNEAS DE UN FRAGMENTO DE LA ENTRADA
    long numeroLineas = 0;
    for(int i=0; i<num_read; i++)
        if(buf[i]== '\n') 
            numeroLineas++;
    return numeroLineas;
}

void wcfd(int fdin, int fdout, char *buf, unsigned buf_size)
{ // TERCER COMANDO
    ssize_t num_read, num_written;
    long numeroLineas = 0;
    long numeroCaracteres = 0;

    while ((num_read = read(fdin, buf, buf_size)) > 0)
    {
        numeroCaracteres += num_read;
        numeroLineas += contarLineas(buf, num_read);
    }
    sprintf(buf, "%ld %ld\n", numeroLineas, numeroCaracteres);
    num_written = write_all(fdout, buf, strlen(buf));
    // como es la salida estandar se podría poner simplemente un printf en vez del sprintf
    // y llamar a la función write_all
    // printf("%ld %ld\n", numeroLineas, numeroCaracteres);
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
    cerrarDescriptor(fdin);
}

void tercerHijo(int pipeTrWc[2])
{ // CIERRA LOS DESCRIPTORES QUE NO NECESITA, LLAMA A wcfd, ENVÍA SU SALIDA A la SALIDA ESTANDAR Y ACABA LA EJECUCIÓN DE ESTE PROCESO
    cerrarDescriptor(pipeTrWc[1]);

    char *bufWc = reservarBuffer(BUF_WC);

    wcfd(pipeTrWc[0], STDOUT_FILENO, bufWc, BUF_WC);

    cerrarDescriptor(STDOUT_FILENO);

    free(bufWc);
    exit(EXIT_SUCCESS);
}
