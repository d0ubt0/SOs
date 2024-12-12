#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>

#define SEM_3_NAME "/sem_3"
#define SEM_4_NAME "/sem_4"
#define SHM_NAME "/my_shared_memory"
#define FIFO_1 "/tmp/mypipe1"
#define FIFO_2 "/tmp/mypipe2"

sem_t *sem_3;
sem_t *sem_4;

int main() {
    
    // Crear memoria compartida
    shm_unlink(SHM_NAME);
    int shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm, sizeof(int)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    int *shm_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    //Eliminar semaforo 3 y 4
    sem_unlink(SEM_3_NAME);
    sem_unlink(SEM_4_NAME);
    
    //Crear semaforo 3
    sem_3 = sem_open(SEM_3_NAME, O_CREAT, 0666, 1);
    if (sem_3 == SEM_FAILED) {
        perror("sem_open sem_3");
        exit(EXIT_FAILURE);
    }
    //Crear semaforo 4
    sem_4 = sem_open(SEM_4_NAME, O_CREAT, 0666, 0);
    if (sem_4 == SEM_FAILED) {
        perror("sem_open sem_4");
        exit(EXIT_FAILURE);
    }

    // Eliminando tuberías por si acaso
    unlink(FIFO_1);
    unlink(FIFO_2);
    
    // Creando tuberías y verificando
    if (mkfifo(FIFO_1, 0666) == -1) {
        perror("mkfifo FIFO_1");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(FIFO_2, 0666) == -1) {
        perror("mkfifo FIFO_2");
        exit(EXIT_FAILURE);
    }

    // Abriendo tuberías
    int fifo_1 = open(FIFO_1,O_WRONLY);
    if (fifo_1 == -1) {
        perror("open FIFO_1");
        exit(EXIT_FAILURE);
    }

    int fifo_2 = open(FIFO_2, O_WRONLY);
    if (fifo_2 == -1) {
        perror("open FIFO_2");
        exit(EXIT_FAILURE);
    }

    int finalizados = 0;
    int senal = -3;
     
    while (finalizados < 2) {
        sem_wait(sem_4); 
        int valor = *shm_ptr;

        if (valor == -1) {
            printf("%d ", valor);
            write(fifo_1, &senal, sizeof(int));
            finalizados++;
        } else if (valor == -2) {
            printf("%d ", valor);
            write(fifo_2, &senal, sizeof(int));
            finalizados++;
        } else {
            printf("%d ", valor);
        }
        sem_post(sem_3);
    }
    printf("\n");

    // Liberar recursos
    munmap(shm_ptr, sizeof(int));
    sem_close(sem_3);
    sem_unlink(SEM_3_NAME);
    shm_unlink(SHM_NAME);
    unlink(FIFO_1);
    unlink(FIFO_2);

    return 0;
}
