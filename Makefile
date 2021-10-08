# PingPongOS - PingPong Operating System
# Tiago Henrique Conte, DINF UFPR
# Setembro de 2021
# Makefile

# define as flags para compilação
CFLAGS = -Wall -std=c99
CC = gcc

OBJS = ppos_core.o queue.o
PROG = pingpong-tasks1 pingpong-tasks2 pingpong-tasks3
 
# regra default
all: testafila $(PROG)
 
# regras de ligacao
testafila: queue.o
$(PROG) : % :  $(OBJS) %.o
	$(CC) $^ -o $@

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
	-rm -f testafila $(PROG)