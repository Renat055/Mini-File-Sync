#ifndef SYNC_H
#define SYNC_H

// Copia un archivo usando solo open, read, write. 
// Retorna la cantidad de bytes copiados, o -1 en caso de error.
long copiarArchivo(const char *origen, const char *destino);

#endif
