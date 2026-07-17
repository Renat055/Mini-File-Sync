// copia archivo grande (argv[1] => argv[2]) sin buffering: O_DIRECT
// el buffer debe estar alineado a 4K (FS block size). El tamanio de read() y write() debe ser multible de BLSZ

#define _GNU_SOURCE // Required for O_DIRECT flag
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define BLOCK_SIZE 4096 // Linux FS block size 

void timeval_sub(struct timeval *result, struct timeval *x, struct timeval *y) {
    // Carry over 1 second if microseconds are negative
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    // Compute the actual difference
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
}
// pkill minisync

int main(int argc, char *argv[]) {
    int fdin, fdout;
    char *buffer;
    struct timeval ts, tf, res;
    ssize_t bytes_read;

    if (argc != 3) {printf ("USO: %s <fnameIN> <fnameOUT>\n", argv[0]); exit (1);}
    
    fdin = open(argv[1], O_RDONLY | O_DIRECT);
    if (fdin < 0) {
        perror("Error opening fdIN with O_DIRECT.");
        return 1;
    }

    fdout = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT | O_DIRECT);
    if (fdout < 0) {
        perror("Error opening fdOUT with O_DIRECT.");
        return 1;
    }

    if (posix_memalign((void **)&buffer, BLOCK_SIZE, BLOCK_SIZE) != 0) {
        perror("Failed to allocate aligned memory");
        close(fdin);
        close(fdout);
        return 1;
    }

    memset(buffer, 0, BLOCK_SIZE);

    gettimeofday (&ts, 0);
    while ((bytes_read = read(fdin, buffer, BLOCK_SIZE)) > 0) {
        write (fdout, buffer, bytes_read);	
	//write(fdout, buffer, BLOCK_SIZE);
    }
    gettimeofday (&tf, 0);

    timeval_sub(&res, &tf, &ts);
    printf ("%ld secs, %ld usecs\n", res.tv_sec, res.tv_usec);

    close(fdin);
    close(fdout);

    return 0;
}
