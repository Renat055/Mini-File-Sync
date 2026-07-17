// * Copiar un archivo a otro. Con FDATASYNC

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
void main(int argc,char **argv) {
  struct timeval ts, tf;
  int fd1, fd2;
  char ch[8192];
  int pos;
  int sz;
  int l;

  if (argc!=4) {printf ("USO: %s <fileSRC> <fileDST> <sz>\n", argv[0]);exit(1);}
 
  gettimeofday (&ts, NULL);
  if ((fd1 = open(argv[1], O_RDONLY)) < 0) {printf("OPEN...\n"); return;}
  fd2 = open(argv[2], O_WRONLY | O_CREAT);  
  sz = atoi(argv[3]);
  pos = lseek(fd1, 0L, SEEK_END); // file pointer at end of file
  printf ("L: %d\n", pos);
  lseek(fd1, 0L, SEEK_SET); // file pointer set at start
  while (pos) {
    l = read (fd1, ch, sz);
    write (fd2, ch, l);
    fdatasync (fd2);
    pos -=l;
  }  
  gettimeofday (&tf, NULL);
  printf ("COPIAR: %f secs\n", ((tf.tv_sec - ts.tv_sec)*1000000u + 
           tf.tv_usec - ts.tv_usec)/ 1.e6);

  close(fd1);  
  close(fd2);  
}
