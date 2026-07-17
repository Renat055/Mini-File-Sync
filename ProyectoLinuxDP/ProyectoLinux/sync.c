#include "sync.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

// Búfer intermedio para almacenar bloques de memoria. 
// 8192 bytes (8KB) es un tamaño estándar muy eficiente para reducir cuellos de botella en E/S.
#define BUF_SIZE 8192 

// Implementación manual de copiado de archivos sin usar herramientas de alto nivel del sistema
long copiarArchivo(const char *origen, const char *destino) {
    
    // 1. ABRIR ORIGEN: Obtenemos el descriptor (fd) del archivo original.
    // Usamos O_RDONLY porque únicamente necesitamos leer de él, no escribir.
    int fd1 = open(origen, O_RDONLY);
    if (fd1 < 0) {
        perror("Error abriendo archivo original");
        return -1; // Retorna error (falla de lectura)
    }

    // ABRIR/CREAR DESTINO: Obtenemos el descriptor del archivo backup.
    // O_WRONLY: Queremos escribirle datos.
    // O_CREAT: Si el archivo no existe en el backup, que el sistema lo cree por nosotros.
    // O_TRUNC: Si el archivo ya existía y tenía datos, que lo deje vacío (0 bytes) para reescribirlo limpio.
    // 0666: Permisos de lectura/escritura predeterminados para el archivo nuevo.
    int fd2 = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 < 0) {
        perror("Error abriendo/creando archivo destino");
        close(fd1); // Cerramos el de origen para no dejarlo colgando
        return -1;
    }

    char ch[BUF_SIZE]; // Nuestro "balde" temporal de 8KB donde pasarán los datos
    
    // MEDICIÓN DEL TAMAÑO 
    // lseek mueve el cursor de lectura del archivo. Le pedimos que se mueva al final absoluto (SEEK_END).
    // La función nos devuelve cuántos bytes tuvo que avanzar, revelándonos el tamaño exacto del archivo.
    long pos = lseek(fd1, 0L, SEEK_END); 
    
    // Luego obligamos al cursor a regresar a la posición 0 (SEEK_SET) para poder leerlo desde el inicio.
    lseek(fd1, 0L, SEEK_SET); 
    
    long total_bytes = pos;
    int l;

    //  BUCLE DE ESCRITURA Y SINCRONIZACIÓN
    // 'pos' actúa como un contador en reversa.
    while (pos > 0) {
        // Leemos hasta 8KB del original y los guardamos en el balde 'ch'
        l = read(fd1, ch, BUF_SIZE);
        if (l <= 0) {
            if (l < 0) {
                perror("Error de lectura en disco (read)");
                total_bytes = -1;
            }
            break; // Si da 0 (fin de archivo) o negativo (error), rompemos el ciclo
        }
        
        // Volcamos exactamente la misma cantidad 'l' de bytes desde el balde hacia el destino
        if (write(fd2, ch, l) != l) {
            perror("Error de escritura (posible disco lleno)");
            total_bytes = -1;
            break;
        }
        
        // SINCRONIZACIÓN FÍSICA FORZADA (fdatasync)

        fdatasync(fd2);
        
        // Restamos lo que ya leímos y escribimos. Si copiamos 8192 bytes, 
        // faltarán 8192 menos en la próxima vuelta, hasta llegar a 0.
        pos -= l;
    }

    // LIMPIEZA
    // Siempre debemos cerrar los descriptores al terminar para devolverle memoria al Kernel.
    close(fd1);
    close(fd2);

    // Retornamos el peso exacto de lo copiado para sumarlo a las estadísticas de la Memoria Compartida
    return total_bytes; 
}
