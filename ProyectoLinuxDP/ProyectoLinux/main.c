#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include "scanner.h"
#include "sync.h"
#include "shared_stats.h"
#include "logger.h"

// Crea toda la ruta de carpetas necesarias si no existen 
void ensure_dir_exists(const char *path) {
    char tmp[8192];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755); 
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

// Convierte el programa en un proceso de segundo plano (Demonio)
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); 

    if (setsid() < 0) exit(EXIT_FAILURE); 

    // signal(SIGCHLD, SIG_IGN); 
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); 

    umask(0); 
   
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <directorio_origen> <directorio_backup>\n", argv[0]);
        return 1;
    }

    char abs_origen[PATH_MAX];
    char abs_backup[PATH_MAX];
    
    if (!realpath(argv[1], abs_origen)) {
        perror("Error con el directorio origen");
        return 1;
    }
    
    mkdir(argv[2], 0755); 
    if (!realpath(argv[2], abs_backup)) {
        perror("Error con el directorio backup");
        return 1;
    }

    daemonize();

    init_shared_stats();
    start_logger();
    
    char log_buf[9000];
    snprintf(log_buf, sizeof(log_buf), "Iniciando monitor: %s -> %s", abs_origen, abs_backup);
    log_message(log_buf);

    FileNode *previous_state = NULL;

    while (1) {
        FileNode *current_state = scan_directory(abs_origen, "");
        
        int changes = 0;
        FileNode *curr = current_state;
        
        while (curr) {
            FileNode *prev = find_file(previous_state, curr->path);
            if (!prev || prev->size != curr->size || prev->mtime != curr->mtime) {
                changes++;
            }
            curr = curr->next;
        }

        if (changes > 0) {
            int num_workers = (changes < 4) ? changes : 4;
            
            printf("[MONITOR] Detectados %d archivos nuevos/modificados. Lanzando %d workers.\n", changes, num_workers);
            fflush(stdout);
            
            // Usaremos un arreglo de pipes. 1 pipe EXCLUSIVO por cada worker.
            // Esto evita que un solo worker se trague (buffer) todas las tareas de golpe.
            int pfds[4][2];
            FILE *streams_write[4];
            pid_t worker_pids[4]; // Guardamos el PID de cada worker para esperarlo específicamente
            
            struct timeval start, end;
            gettimeofday(&start, NULL); // Cronómetro de rendimiento

            for (int i = 0; i < num_workers; i++) {
                if (pipe(pfds[i]) == -1) {
                    perror("Error al crear pipe");
                    continue;
                }
                
                pid_t pid = fork();
                
                if (pid == 0) {
                    // ==========================================
                    // ZONA DEL WORKER (PROCESO HIJO)
                    // ==========================================
                    
                    // El hijo cierra los extremos de los pipes que no usa
                    for (int j = 0; j <= i; j++) {
                        close(pfds[j][1]); // Cierra salida de escritura de su pipe y de los workers anteriores
                        
                    }
                    
                    int my_worker_id = i + 1; // ID: 1, 2, 3 o 4
                    char buffer[2048];
                    FILE *stream = fdopen(pfds[i][0], "r"); // El worker escucha solo de SU propio pipe
                    
                    while (fgets(buffer, sizeof(buffer), stream) != NULL) {
                        buffer[strcspn(buffer, "\n")] = 0;
                        
                        if (strncmp(buffer, "COPIAR ", 7) == 0) {
                            char *archivo = buffer + 7;
                            
                            printf("[WORKER %d] Recibió orden de copiar: %s\n", my_worker_id, archivo);
                            fflush(stdout);
                            
                            char ruta_origen[8192];
                            char ruta_destino[8192];
                            snprintf(ruta_origen, sizeof(ruta_origen), "%s/%s", abs_origen, archivo);
                            snprintf(ruta_destino, sizeof(ruta_destino), "%s/%s", abs_backup, archivo);
                            
                            char ruta_dir[8192];
                            snprintf(ruta_dir, sizeof(ruta_dir), "%s/%s", abs_backup, archivo);
                            char *last_slash = strrchr(ruta_dir, '/');
                            if (last_slash) {
                                *last_slash = '\0';
                                ensure_dir_exists(ruta_dir); 
                            }
                            
                            long bytes = copiarArchivo(ruta_origen, ruta_destino);
                            
                            time_t t = time(NULL);
                            struct tm *tm_info = localtime(&t);
                            char time_buf[26];
                            strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);

                            char msg[512];
                            if (bytes >= 0) {
                                printf("[%s] worker%d: copiado %s\n", time_buf, my_worker_id, ruta_origen);
                                fflush(stdout); // Imprime directo en consola
                                
                                snprintf(msg, sizeof(msg), "copiado %s", archivo);
                                update_stats(bytes, 0); 
                            } else {
                                snprintf(msg, sizeof(msg), "error copiando %s", archivo);
                                update_stats(0, 1); 
                            }
                            log_message(msg);
                        }
                    }
                    fclose(stream);
                    exit(0);
                } else {

                    // ZONA DEL MONITOR (PADRE) - Preparando pipes

                    worker_pids[i] = pid; // Guardamos su PID oficial
                    close(pfds[i][0]); // Padre no lee, cierra lectura
                    streams_write[i] = fdopen(pfds[i][1], "w");
                }
            } // Fin de for de creación

            // Repartimos los archivos a los workers 
      
            curr = current_state;
            int w_idx = 0;
            while (curr) {
                FileNode *prev = find_file(previous_state, curr->path);
                if (!prev || prev->size != curr->size || prev->mtime != curr->mtime) {
                    printf("[MONITOR] Asignando %s al Worker %d\n", curr->path, w_idx + 1);
                    fflush(stdout);
                    fprintf(streams_write[w_idx], "COPIAR %s\n", curr->path); 
                    w_idx = (w_idx + 1) % num_workers; 
                }
                curr = curr->next;
            }
            
            // Sellamos todos los pipes para que los workers sepan que no hay más trabajo
            for (int i = 0; i < num_workers; i++) {
                fclose(streams_write[i]);
            }
            
            // Sincronización: El Monitor espera EXCLUSIVAMENTE a los workers por su PID exacto.
            // Esto previene que se confunda con otros procesos hijos y se quede colgado.
            for (int w = 0; w < num_workers; w++) {
                waitpid(worker_pids[w], NULL, 0);
            }
            
            // Calculamos el tiempo total de la operación (Rendimiento)
            gettimeofday(&end, NULL);
            double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
            if (elapsed < 0.001) elapsed = 0.001; // Para evitar dividir entre cero si fue instantáneo
            add_time_stats(elapsed);
        } 
        
        if (previous_state) free_file_list(previous_state);
        previous_state = current_state;
        
        print_stats();
        sleep(5);
    }
    
    destroy_shared_stats();
    destroy_logger();
    return 0;
}
