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

void limpiar_recursos(int fifo_1, int fifo_2, int shm, int creador_semaforos) {
    if (fifo_1 >= 0) close(fifo_1);
    if (fifo_2 >= 0) close(fifo_2);
    if (shm >= 0) close(shm);

    if (sem_3) sem_close(sem_3);
    if (sem_4) sem_close(sem_4);

    if (creador_semaforos) {
        sem_unlink(SEM_3_NAME);
        sem_unlink(SEM_4_NAME);
    }

    shm_unlink(SHM_NAME);
    unlink(FIFO_1);
    unlink(FIFO_2);
}

int main() {

    //Crear memoria
    shm_unlink(SHM_NAME);
    int shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    //Tamano memoria
    if (ftruncate(shm, sizeof(int)) == -1) {
        perror("ftruncate");
        limpiar_recursos(-1, -1, shm, 0);
        exit(EXIT_FAILURE);
    }
    //Puntero memoria
    int *shm_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        limpiar_recursos(-1, -1, shm, 0);
        exit(EXIT_FAILURE);
    }

    //Borrar semaforos anteriores por si acaso
    sem_unlink(SEM_3_NAME);
    sem_unlink(SEM_4_NAME);

    //Abrir semaforo 3
    sem_3 = sem_open(SEM_3_NAME, O_CREAT, 0666, 1);
    if (sem_3 == SEM_FAILED) {
        perror("sem_open sem_3");
        limpiar_recursos(-1, -1, shm, 0);
        exit(EXIT_FAILURE);
    }
    //Abrir semaforo  4
    sem_4 = sem_open(SEM_4_NAME, O_CREAT, 0666, 0);
    if (sem_4 == SEM_FAILED) {
        perror("sem_open sem_4");
        limpiar_recursos(-1, -1, shm, 1);
        exit(EXIT_FAILURE);
    }

    //Borrar tuberias anteriores por si acaso
    unlink(FIFO_1);
    unlink(FIFO_2);

    //Crear tuberia 1
    if (mkfifo(FIFO_1, 0666) == -1) {
        perror("mkfifo FIFO_1");
        limpiar_recursos(-1, -1, shm, 1);
        exit(EXIT_FAILURE);
    }
    //Crear tuberia 2
    if (mkfifo(FIFO_2, 0666) == -1) {
        perror("mkfifo FIFO_2");
        limpiar_recursos(-1, -1, shm, 1);
        exit(EXIT_FAILURE);
    }

    //Abrir tuberia 1 en mood escritura
    int fifo_1 = open(FIFO_1, O_WRONLY);
    if (fifo_1 == -1) {
        perror("open FIFO_1");
        limpiar_recursos(-1, -1, shm, 1);
        exit(EXIT_FAILURE);
    }
    //Abrir tuberia 2 en modo escritura
    int fifo_2 = open(FIFO_2, O_WRONLY);
    if (fifo_2 == -1) {
        perror("open FIFO_2");
        limpiar_recursos(fifo_1, -1, shm, 1);
        exit(EXIT_FAILURE);
    }

    printf("Esperando por P1 y P2\n");

    int finalizados = 0;
    int senal = -3;

    //Ciclo de espera y lectura a P1_P2

    while (finalizados < 2) {
        sem_wait(sem_4);
        int valor = *shm_ptr;
        printf("%d ", valor);

        if (valor == -1) {
            write(fifo_1, &senal, sizeof(int));
            finalizados++;
        } else if (valor == -2) {
            write(fifo_2, &senal, sizeof(int));
            finalizados++;
        }
        sem_post(sem_3);
    }

    printf("\n");

    //Borrando recursos creados
    munmap(shm_ptr, sizeof(int));
    limpiar_recursos(fifo_1, fifo_2, shm, 1);

    return 0;
}
