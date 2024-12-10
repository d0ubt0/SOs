#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>

sem_t *sem_3;

int main() {
    
    // Crear memoria compartida
    shm_unlink("/shm");
    int shm = shm_open("/shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm, sizeof(int));
    int *shm_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    // Crear semáforo
    sem_unlink("/sem_3");
    sem_3 = sem_open("/sem_3", O_CREAT, 0666, 0);

    char fifodir1[] = "/fifo_1";
    char fifodir2[] = "/fifo_2";
    
    mkfifo(fifodir1, 0666);
    mkfifo(fifodir2, 0666);

    int fifo_1 = open(fifodir1, O_WRONLY);
    int fifo_2 = open(fifodir2, O_WRONLY);

    int finalizados = 0;

    while (finalizados < 2) {
        sem_wait(sem_3); // Esperar datos
        int valor = *shm_ptr;

        int senal = -3; // Valor de señalización para indicar finalización

        if (valor == -1) {
            printf("Proceso de pares finalizó.\n");
            write(fifo_1, &senal, sizeof(int));
            finalizados++;
        } else if (valor == -2) {
            printf("Proceso de impares finalizó.\n");
            write(fifo_2, &senal, sizeof(int));
            finalizados++;
        } else {
            printf("Consumidor recibió: %d\n", valor);
        }
    }

    // Liberar recursos
    munmap(shm_ptr, sizeof(int));
    sem_close(sem_3);
    sem_unlink("/sem_3");
    shm_unlink("/shm");
    unlink(fifodir1);
    unlink(fifodir2);

    return 0;
}
