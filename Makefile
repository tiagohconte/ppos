# PingPongOS - PingPong Operating System
# Tiago Henrique Conte, DINF UFPR
# Setembro de 2021
# Makefile

# define as flags para compilação
CFLAGS = -Wall -std=c99
CC = gcc

BINARIES = testafila
 
objs = queue.o
libs = queue.h
 
# regra default
all: $(BINARIES)
 
# regras de ligacao
testafila: $(objs) testafila.o
	$(CC) $(CFLAGS) -o $@ $^

# regras de compilação
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

# compila com flags de depuração
debug: CFLAGS += -DDEBUG -g
debug: all

# remove arquivos temporários
clean:
	-rm -f *.o

# remove tudo o que não for o código-fonte
purge: clean
	-rm -f $(BINARIES)