#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// Esta función se encarga de explorar todas las carpetas, como si fuera el comando 'ls' o 'dir'
// y almacena las propiedades (metadatos) de cada archivo en una cadena de memoria (Lista Enlazada).
FileNode* scan_directory(const char *base_dir, const char *current_dir) {
    FileNode *head = NULL; // 'head' apunta al primer archivo que encontremos
    FileNode *tail = NULL; // 'tail' apunta al último, para no tener que recorrer toda la lista al agregar nuevos
    
    char search_path[9000];
    // Unimos el directorio base con el subdirectorio actual donde estemos buscando
    if (current_dir[0] == '\0') {
        snprintf(search_path, sizeof(search_path), "%s", base_dir); // Estamos en la raíz del origen
    } else {
        snprintf(search_path, sizeof(search_path), "%s/%s", base_dir, current_dir); // Estamos dentro de una subcarpeta
    }

    
    // opendir() le pide permiso al sistema operativo para leer el índice de la carpeta
    DIR *dir = opendir(search_path);
    if (!dir) {
        perror("Error accediendo al directorio");
        return NULL;
    }

    struct dirent *entry;
   
    // readdir() nos devuelve archivo por archivo hasta que no quede ninguno (NULL)
    while ((entry = readdir(dir)) != NULL) {
        
      
       
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[10000]; // Ruta completa en el disco duro
        snprintf(full_path, sizeof(full_path), "%s/%s", search_path, entry->d_name);
        
        char rel_path[10000];  // Ruta relativa (solo la parte de la subcarpeta hacia adentro)
        if (current_dir[0] == '\0') {
            snprintf(rel_path, sizeof(rel_path), "%s", entry->d_name);
        } else {
            snprintf(rel_path, sizeof(rel_path), "%s/%s", current_dir, entry->d_name);
        }

        struct stat st;

        // lstat "mira la etiqueta" del archivo sin tener que abrir el archivo real,
        // dándonos toda la información de su i-nodo.
        if (lstat(full_path, &st) == 0) {
            
      
            if (S_ISDIR(st.st_mode)) {
                //La función se llama a sí misma para entrar a investigar esa subcarpeta.
                FileNode *sub_list = scan_directory(base_dir, rel_path);
                
                // Si encontró archivos adentro, los pegamos (empalmamos) al final de nuestra lista actual.
                if (sub_list) {
                    if (!head) {
                        head = sub_list;
                        tail = head;
                    } else {
                        tail->next = sub_list;
                    }
                    // Avanzamos 'tail' hasta el verdadero final para que quede listo
                    while (tail && tail->next) {
                        tail = tail->next;
                    }
                }
            } 
    
            else if (S_ISREG(st.st_mode)) {
                
                // Pedimos un pedazo de memoria RAM dinámicamente para guardar los datos de este archivo
                FileNode *node = malloc(sizeof(FileNode));
                
                // Copiamos su ruta
                strncpy(node->path, rel_path, sizeof(node->path) - 1);
                node->path[sizeof(node->path) - 1] = '\0'; // Aseguramos que termine correctamente
                
                // Guardamos los datos críticos para que el Monitor pueda compararlos luego
                node->inode = st.st_ino;     // Identificador único interno en Linux
                node->size = st.st_size;     // Su peso en bytes (si alguien lo edita, cambiará)
                node->mode = st.st_mode;     // Permisos (lectura, escritura, ejecución)
                node->mtime = st.st_mtime;   // Fecha y hora del último cambio detectado
                node->next = NULL;

                // Lo enganchamos al final de nuestra cadena (Lista Enlazada)
                if (!head) {
                    head = node;
                    tail = head;
                } else {
                    tail->next = node; // El último elemento ahora apunta al nuevo
                    tail = node;       // El nuevo se convierte en el nuevo último elemento
                }
            }
        }
    }
    closedir(dir); // Cerramos el libro del directorio
    return head; // Devolvemos el eslabón principal de la cadena (el apuntador que sujeta a toda la lista)
}

// Libera toda la RAM utilizada por los FileNodes cuando ya no los ocupamos.

void free_file_list(FileNode *head) {
    while (head) {
        FileNode *temp = head; // Guardamos el actual
        head = head->next;     // Saltamos al siguiente
        free(temp);            // Destruimos el actual
    }
}

// Búsqueda lineal en la lista enlazada. Recorre eslabón por eslabón hasta
// encontrar el archivo que coincida con el nombre que buscamos.
FileNode* find_file(FileNode *head, const char *path) {
    while (head) {
        if (strcmp(head->path, path) == 0) {
            return head; // Lo encontramos
        }
        head = head->next;
    }
    return NULL; // No existe en la lista (significa que es un archivo totalmente nuevo)
}
