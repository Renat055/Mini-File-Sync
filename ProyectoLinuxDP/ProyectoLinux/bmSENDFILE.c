// copia archivo grande (argv[1] => argv[2]) con sendfile()
// el buffer debe estar alineado a 4K (FS block size)

#define _GNU_SOURCE // Required for O_DIRECT flag
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

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

int main(int argc, char *argv[]) {
    int fdin, fdout;
    char *buffer;
    struct timeval ts, tf, res;
    ssize_t bytes_read;

    if (argc != 3) {printf ("USO: %s <fnameIN> <fnameOUT>\n", argv[0]); exit (1);}
    
    fdin = open(argv[1], O_RDONLY);
    if (fdin < 0) {
        perror("Error opening fdIN.");
        return 1;
    }

    fdout = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT);
    if (fdout < 0) {
        perror("Error opening fdOUT.");
        return 1;
    }
    // Get size of source file
    struct stat st;
    if (fstat(fdin, &st) < 0) {
        perror("fstat");
        close(fdin);
        close(fdout);
        return 1;
    }

    gettimeofday (&ts, 0);
    // Use sendfile to copy data from src_fd to dst_fd
    off_t offset = 0;
    ssize_t bytes_sent;

    while (offset < st.st_size) {
        bytes_sent = sendfile(fdout, fdin, &offset, st.st_size - offset);
        if (bytes_sent <= 0) {
            if (errno == EINTR)
                continue;
            perror("sendfile");
            close(fdin);
            close(fdout);
            return 1;
        }
    } 
    gettimeofday (&tf, 0);

    timeval_sub(&res, &tf, &ts);
    printf ("%ld secs, %ld usecs\n", res.tv_sec, res.tv_usec);

    close(fdin);
    close(fdout);

    return 0;
}
