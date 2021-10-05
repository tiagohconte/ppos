# PingPongOS - PingPong Operating System
# Tiago Henrique Conte, DINF UFPR
# Setembro de 2021
# Makefile

# define as flags para compilação
CFLAGS = -Wall -std=c99
CC = gcc

BINARIES = testafila pingpong-tasks1 pingpong-tasks2 pingpong-tasks3
 
objs = ppos_core.o queue.o
libs = ppos_core.h queue.h
 
# regra default
all: $(BINARIES)
 
# regras de ligacao
testafila: queue.o
pingpong-tasks1: $(objs) pingpong-tasks1.o
pingpong-tasks2: $(objs) pingpong-tasks2.o
pingpong-tasks3: $(objs) pingpong-tasks3.o

# regras de compilação
%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

# compila com flags de depuração
debug: CFLAGS += -DDEBUG -g
debug: all

# remove arquivos temporários
clean:
	-rm -f *.o

# remove tudo o que não for o código-fonte
purge: clean
	-rm -f $(BINARIES)