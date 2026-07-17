CC=gcc
CFLAGS=-Wall -g -pthread
LDFLAGS=-lrt -pthread

OBJ=main.o scanner.o sync.o shared_stats.o logger.o
TARGET=minisync

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
