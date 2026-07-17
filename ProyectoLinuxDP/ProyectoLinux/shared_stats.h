#ifndef SHARED_STATS_H
#define SHARED_STATS_H

#include <semaphore.h>

#define SHM_NAME "/minisync_shm"
#define SEM_NAME "/minisync_sem"

struct stats {
    long archivos_copiados;
    long bytes_copiados;
    long errores;
    double tiempo_total_segundos; // Acumulador de tiempo para calcular rendimiento
};

// Funciones de inicialización y limpieza de la memoria RAM compartida
void init_shared_stats();
void destroy_shared_stats();

// Funciones para interactuar con la pizarra de estadísticas
void print_stats();
void update_stats(long bytes, int error);

// Función para sumar el tiempo transcurrido en el último lote de workers
void add_time_stats(double seconds);

#endif
