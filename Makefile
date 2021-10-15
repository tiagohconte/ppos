# PingPongOS - PingPong Operating System
# Tiago Henrique Conte, DINF UFPR
# Setembro de 2021
# Makefile

# define as flags
CC = gcc
CFLAGS = -Wall
LFLAGS = -lm

OBJS = ppos_core.o queue.o
PROG = pingpong-preempcao pingpong-preempcao-stress
 
# regra default
all: $(PROG)
 
# regras de ligacao
$(PROG) : % :  $(OBJS) %.o
	$(CC) $^ -o $@ $(LFLAGS)

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
	-rm -f $(PROG)