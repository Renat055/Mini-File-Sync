#ifndef SCANNER_H
#define SCANNER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

typedef struct FileNode {
    char path[1024];     // Ruta relativa al directorio monitorizado
    ino_t inode;         // Numero de inodo
    off_t size;          // Tamaño en bytes
    mode_t mode;         // Permisos
    time_t mtime;        // Fecha de modificación
    struct FileNode *next;
} FileNode;

// Escanea un directorio recursivamente y devuelve una lista enlazada
FileNode* scan_directory(const char *base_dir, const char *current_dir);

// Libera la memoria de la lista
void free_file_list(FileNode *head);

// Busca un archivo por su ruta relativa
FileNode* find_file(FileNode *head, const char *path);

#endif
