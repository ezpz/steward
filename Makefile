GCC = gcc
GPP = g++
WFLAGS = -W -Wall -Wextra -Werror
DFLAGS = -g
OFLAGS = -O3 -ansi
IFLAGS = -I./include
FLAGS = $(WFLAGS) $(DFLAGS) $(OFLAGS) $(IFLAGS)

STEWARD = steward
SERVER = server
CLIENT = client

# _GNU_SOURCE enables RTLD_NEXT
MACROS = -D_GNU_SOURCE

NTRACE_LIB = ntrace
NTRACE_SRC = src/ntrace
NTRACE_INC = include/ntrace
LINK = -ldl 
LIB_FILE = lib$(NTRACE_LIB).so

LIB_SRCS = $(NTRACE_SRC)/ntrace.c \
		   $(NTRACE_SRC)/callback.c \
		   $(NTRACE_SRC)/util.c

default: $(SERVER) $(CLIENT) $(STEWARD) $(LIB_FILE)

$(LIB_FILE): $(LIB_SRCS)
	$(GCC) $(MACROS) $(FLAGS) -fPIC -shared -Wl,-soname,$(LIB_FILE) -I$(NTRACE_INC) -o $@ $(LINK) $^

$(SERVER): src/server.cc
	$(GPP) $(FLAGS) -o $@ $^ -lpthread -lsqlite3 

$(CLIENT): src/test_client.c
	$(GCC) $(FLAGS) -o $@ $^

$(STEWARD): src/steward.cc
	$(GPP) $(FLAGS) -o $@ $^ -lncurses

clean:
	rm -f $(SERVER) $(CLIENT) $(STEWARD) $(LIB_FILE)
	
