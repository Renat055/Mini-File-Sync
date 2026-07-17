#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <time.h>

void start_logger() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    
    mq_unlink(MQ_NAME);
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open start_logger");
        exit(1);
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork logger");
        exit(1);
    }
    
    if (pid == 0) {
        FILE *log_file = fopen("sync_log.txt", "a");
        if (!log_file) {
            perror("fopen logger");
            exit(1);
        }

        char buffer[MAX_MSG_SIZE];
        while (1) {
            ssize_t bytes_read = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);
            if (bytes_read >= 0) {
                buffer[bytes_read] = '\0';
                time_t t = time(NULL);
                struct tm *tm_info = localtime(&t);
                char time_buf[26];
                strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
                
                fprintf(log_file, "[%s] %s\n", time_buf, buffer);
                fflush(log_file);
            }
        }
        fclose(log_file);
        exit(0);
    }
    mq_close(mq); 
}

void destroy_logger() {
    mq_unlink(MQ_NAME);
}

void log_message(const char *msg) {
    // Usamos O_NONBLOCK para evitar que un worker se cuelgue si el Logger dejó de leer mensajes
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY | O_NONBLOCK);
    if (mq != (mqd_t)-1) {
        mq_send(mq, msg, strlen(msg), 0);
        mq_close(mq);
    }
}
