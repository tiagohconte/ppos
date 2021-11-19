// PingPongOS - PingPong Operating System
// GRR20190374 - Tiago Henrique Conte, DINF UFPR
// Novembro de 2021

// Teste de semáforos

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
typedef struct {
  struct item_t *prev, *next;		// ponteiros para usar em filas
  int valor;   // armazena o item
} item_t;

task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;
item_t *buffer = NULL;

void print_item (void *ptr) {

  item_t *item = ptr ;

  if (!item)
    return ;

  printf ("%d", item->valor) ;

}

// insere item no buffer
void insere_buffer(int valor) {
  item_t *item = (item_t *) malloc( sizeof(item_t) );
  item->prev = NULL;
  item->next = NULL;
  item->valor = valor;

  if ( queue_append ((queue_t **) &buffer, (queue_t*) item) )
    exit(-1);
}

// remove primeiro item do buffer e retorna valor
int remove_buffer() {
  item_t *item = buffer;

  if ( queue_remove ((queue_t **) &buffer, (queue_t*) item) )
    exit(-1);

  int valor = item->valor;
  free(item);

  return(valor);
}

// produtor
void produtor(void * arg) {
  int valor;

  while(1) {
    task_sleep(1000);
    valor = rand() % 100;

    sem_down( &s_vaga );

    sem_down( &s_buffer );
    insere_buffer(valor);
    sem_up( &s_buffer );

    sem_up( &s_item );

    fprintf(stdout, "%s produziu %d (tem %d)\n", (char *) arg, valor, queue_size( (queue_t *) buffer ));

  }

  task_exit (0) ;
  
}

// consumidor
void consumidor(void * arg) {
  int valor;

  while(1) {
    sem_down( &s_item );

    sem_down( &s_buffer );    
    valor = remove_buffer();  
    sem_up( &s_buffer );

    sem_up( &s_vaga );

    fprintf(stdout, "%s consumiu %d (tem %d)\n", (char *) arg, valor, queue_size( (queue_t *) buffer ));

    task_sleep(1000);

  }

  task_exit (0) ;
}

int main(int argc, char const *argv[]) {

  ppos_init ();

  // cria semaforos
  sem_create(&s_buffer, 1);
  sem_create(&s_item, 0);
  sem_create(&s_vaga, 5);

  // cria tarefas
  task_create (&p1, produtor, "P1");
  task_create (&p2, produtor, "  P2");
  task_create (&p3, produtor, "    P3");
  task_create (&c1, consumidor, "                             C1");
  task_create (&c2, consumidor, "                               C2");

  // aguarda tasks encerrarem
  task_join (&p1);
  task_join (&p2);
  task_join (&p3);
  task_join (&c1);
  task_join (&c2);

  // printf ("main: fim\n");
  task_exit (0);
  
  return 0;
}
