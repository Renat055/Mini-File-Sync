#ifndef LOGGER_H
#define LOGGER_H

#define MQ_NAME "/minisync_mq"
#define MAX_MSG_SIZE 256

// Inicializa el logger y arranca su bucle infinito
void start_logger();

// Funciones de limpieza (para el proceso principal antes de salir)
void destroy_logger();

// Envia mensaje al logger
void log_message(const char *msg);

#endif
