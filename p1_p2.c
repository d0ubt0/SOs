#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#define SEM_1_NAME "/sem_1"
#define SEM_2_NAME "/sem_2"
#define SEM_3_NAME "/sem_3"
#define SEM_4_NAME "/sem_4"
#define SHM_NAME "/my_shared_memory"
#define FIFO_1 "/tmp/mypipe1"
#define FIFO_2 "/tmp/mypipe2"

sem_t *sem_1; 
sem_t *sem_2; 
sem_t *sem_3; 
sem_t *sem_4;

void generar_pares(int n, int inicio, int *ptr) {
    for (int i = 0; i < n; i++) {
        sem_wait(sem_1);
        sem_wait(sem_3);
        *ptr = inicio + (i * 2); 
        sem_post(sem_4);
        sem_post(sem_2);        
    }

    sem_wait(sem_1);
    sem_wait(sem_3);
    *ptr = -1; 
    sem_post(sem_4);         
    sem_post(sem_2); 
}

void generar_impares(int n, int inicio, int *ptr) {
    for (int i = 0; i < n; i++) {
        sem_wait(sem_2);
        sem_wait(sem_3);    
        *ptr = inicio + (i * 2); 
        sem_post(sem_4);
        sem_post(sem_1);        
    }

    sem_wait(sem_2);
    sem_wait(sem_3);
    *ptr = -2; 
    sem_post(sem_4);        
    sem_post(sem_1); 
}

int main(int argc, char *argv[]) {
    // Verificar cantidad de argumentos
    if (argc != 4) {
        perror("Cantidad de argumentos inválida");
        exit(1);
    }

    int n = atoi(argv[1]);
    int a1 = atoi(argv[2]);
    int a2 = atoi(argv[3]);
    
    // Verificar inputs
    if (n < 1 || a1 % 2 != 0 || a2 % 2 == 0) {
        perror("Valores incorrectos");
        exit(1);
    }

    // Verificar si P3 ya fue inicializado por el p3
    sem_3 = sem_open(SEM_3_NAME, 0);
    if (sem_3 == SEM_FAILED) {
        perror("Primero inicie P3");
        exit(1);
    }
    sem_4 = sem_open(SEM_4_NAME, 0);
    if (sem_4 == SEM_FAILED) {
        perror("Error al abrir semáforo SEM_4");
        sem_close(sem_3);
        exit(EXIT_FAILURE);
    }

    // Eliminando y luego creando semáforos
    sem_unlink(SEM_1_NAME);
    sem_unlink(SEM_2_NAME);

    srand(time(NULL));
    int boolean = rand() % 2;

    if (boolean == 0) {
        sem_1 = sem_open(SEM_1_NAME, O_CREAT, 0666, 1);  
        sem_2 = sem_open(SEM_2_NAME, O_CREAT, 0666, 0); 
    } else {
        sem_2 = sem_open(SEM_2_NAME, O_CREAT, 0666, 1); 
        sem_1 = sem_open(SEM_1_NAME, O_CREAT, 0666, 0);
    }

    //Verificando semaforos creados
    if (sem_1 == SEM_FAILED || sem_2 == SEM_FAILED) {
        perror("Error al crear semáforos SEM_1 o SEM_2");
        exit(EXIT_FAILURE);
    }

    // Abriendo memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("Error al abrir memoria compartida");
        exit(EXIT_FAILURE);
    }

    //Mapeando puntero
    int *shm_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Error al mapear memoria compartida");
        exit(EXIT_FAILURE);
    }

    //Abriendo tuberias creadas por P3
    int fifo_1 = open(FIFO_1, O_RDONLY);
    int fifo_2 = open(FIFO_2, O_RDONLY);

    if (fifo_1 < 0 || fifo_2 < 0) {
        perror("Error al abrir FIFOs");
        exit(EXIT_FAILURE);
    }


    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        exit(1);
    }

    if (pid == 0) {
        generar_impares(n, a2, shm_ptr);

        int senal;
        read(fifo_2, &senal, sizeof(senal));
        if (senal == -3) {
            printf("-3 P2 termina\n");
        }
        
        close(fifo_2);
        unlink(FIFO_2);
        return 0;

    } else {
        generar_pares(n, a1, shm_ptr);

        int senal;
        read(fifo_1, &senal, sizeof(senal));
        if (senal == -3) {
            printf("-3 P1 termina\n");
        }
        close(fifo_1);
        unlink(FIFO_1);

        wait(NULL);

        sem_close(sem_1);
        sem_close(sem_2);
        sem_unlink(SEM_1_NAME);
        sem_unlink(SEM_2_NAME);
    }

    return 0;
}
