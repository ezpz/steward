GCC = gcc
GPP = g++
WFLAGS = -W -Wall -Wextra -Werror
DFLAGS = -g
OFLAGS = -O3 -ansi
IFLAGS = -I./include
FLAGS = $(WFLAGS) $(DFLAGS) $(OFLAGS)

SERVER = server
CLIENT = client

default: $(SERVER) $(CLIENT)

$(SERVER): src/server.cc
	$(GPP) $(FLAGS) -o $@ $^ -lpthread -lsqlite3 

$(CLIENT): src/test_client.c
	$(GCC) $(FLAGS) -o $@ $^

clean:
	rm -f $(SERVER) $(CLIENT)
	
