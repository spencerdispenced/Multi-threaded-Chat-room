CC = gcc
CFLAGS = -Wall -Werror -pedantic 
DEBUG_FLAGS = -g
STD = -std=c11 -D_POSIX_C_SOURCE=200112L
LIBS = -lpthread


all:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(STD) $(LIBS) chat_server.c -o chat_server
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(STD) $(LIBS) chat_client.c -o chat_client


clean:
	rm  *chat_client chat_server *.log




