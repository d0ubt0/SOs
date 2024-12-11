#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
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
    //Verificar cantidad argumentos
    if (argc != 4) {
        perror("Cantidad argumentos invalidas");
        exit(1);
    }

    int n = atoi(argv[1]);
    int a1 = atoi(argv[2]);
    int a2 = atoi(argv[3]);
	

    //Verificar inputs
    if (n < 1 || a1 % 2 != 0 || a2 & 2 == 0) {
        perror("Valores incorrectos");
        exit(1);
    }

    //Verificar si P3 ya fue inicalizado
    sem_3 = sem_open("/sem_3", 0);
    if (sem_3 == SEM_FAILED) {
        perror("Primero inicie P3");
        exit(1);
    }
    sem_4 = sem_open("/sem_4",0);
    //Creando semaforos
    sem_unlink("/sem_1");
    sem_unlink("/sem_2");

    srand(time(NULL));

    int boolean = rand() % 2;

    if( boolean == 0){
        sem_1 = sem_open("/sem_1", O_CREAT, 0666, 1);  
        sem_2 = sem_open("/sem_2", O_CREAT, 0666, 0); 
    } 
    else{
        sem_2 = sem_open("/sem_2", O_CREAT, 0666, 1); 
        sem_1 = sem_open("/sem_1", O_CREAT, 0666,0);
    }
 
    //Abriendo memoria compartida
    int shm = shm_open("/shm", O_RDWR, 0666);
    int *shm_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
   

    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        exit(1);
    }

    if (pid == 0) {
        generar_impares(n, a2, shm_ptr);

        int fifo_2 = open("/fifo_2", O_RDONLY);
        int senal;
        
        read(fifo_2, &senal, sizeof(senal));
        if (senal == -3) {
            printf("-3 P2 termina\n");
        }
        close(fifo_2);
        unlink("/fifo_2");
        return 0;

    } else {
        generar_pares(n, a1, shm_ptr);

        int fifo_1 = open("/fifo_1", O_RDONLY);
	int senal;
        read(fifo_1, &senal, sizeof(int));

        printf("Senal P1 %d\n", senal);
        if (senal == -3) {
            printf("-3 P1 termina\n");
        }
        close(fifo_1);
        unlink("/fifo_1");

        wait(NULL);

        sem_close(sem_1);
        sem_close(sem_2);
        sem_unlink("/sem_1");
        sem_unlink("/sem_2");
    }

    return 0;
}
