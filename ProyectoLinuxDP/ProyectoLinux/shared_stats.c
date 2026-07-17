#include "shared_stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

static struct stats *shm_stats = NULL;
static sem_t *sem = NULL;

void init_shared_stats() {
    shm_unlink(SHM_NAME);
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    
    if (ftruncate(fd, sizeof(struct stats)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    
    shm_stats = mmap(NULL, sizeof(struct stats), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_stats == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(fd);

    shm_stats->archivos_copiados = 0;
    shm_stats->bytes_copiados = 0;
    shm_stats->errores = 0;
    shm_stats->tiempo_total_segundos = 0.0;

    sem_unlink(SEM_NAME);
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
}

void destroy_shared_stats() {
    if (shm_stats != MAP_FAILED) munmap(shm_stats, sizeof(struct stats));
    if (sem != SEM_FAILED) sem_close(sem);
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
}

void print_stats() {
    if (sem && shm_stats != MAP_FAILED) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_buf[26];
        strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        sem_wait(sem);
        
        double kb_s = 0.0;
        double f_s = 0.0;
        if (shm_stats->tiempo_total_segundos > 0) {
            // Pasamos a KB/s porque los archivos de prueba pesan muy pocos bytes
            // Si usamos MB/s, 300 bytes equivalen a 0.00 MB/s.
            kb_s = (shm_stats->bytes_copiados / 1024.0) / shm_stats->tiempo_total_segundos;
            f_s = shm_stats->archivos_copiados / shm_stats->tiempo_total_segundos;
        }

        // Mostrará rendimiento en KB/s en lugar de MB/s para que sea visible
        printf("[%s] stats -> copiados: %ld | bytes: %ld | errores: %ld | Rendimiento: %.2f KB/s | %.2f arch/s\n", 
               time_buf,
               shm_stats->archivos_copiados, 
               shm_stats->bytes_copiados, 
               shm_stats->errores,
               kb_s, f_s);
        fflush(stdout);
        
        sem_post(sem);
    }
}

void update_stats(long bytes, int error) {
    if (sem && shm_stats != MAP_FAILED && shm_stats != NULL) {
        sem_wait(sem);
        
        if (error) {
            shm_stats->errores++;
        } else {
            shm_stats->archivos_copiados++;
            shm_stats->bytes_copiados += bytes;
        }
        
        sem_post(sem);
    }
}

void add_time_stats(double seconds) {
    if (sem && shm_stats != MAP_FAILED && shm_stats != NULL) {
        sem_wait(sem);
        shm_stats->tiempo_total_segundos += seconds;
        sem_post(sem);
    }
}
