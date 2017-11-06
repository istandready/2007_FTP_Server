CC = gcc 

INCLUDE = \
-I /kong_temp/

BIN_DIR = ./

SRC_DIR = /kong_temp/

CFLAGS = -g -Wall -DDEBUG_FILE -DDEBUG -DSEE_MALLOC

CLIBS = \
-lpthread 


TARGET_O = \
$(SRC_DIR)ftp_session.o \
$(SRC_DIR)main.o \
$(SRC_DIR)shm.o \
$(SRC_DIR)file_operate.o \
$(SRC_DIR)env_and_log.o

.c.o: 
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

all: ftp

ftp: $(TARGET_O)
	$(CC) $(TARGET_O) -o $(BIN_DIR)$@ $(CLIBS)

clean: 
	rm -f $(TARGET_O) ftp


