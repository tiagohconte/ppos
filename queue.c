// PingPongOS - PingPong Operating System
// Tiago Henrique Conte, DINF UFPR
// Setembro de 2021
// Implementação fila genérica

#include <stdio.h>
#include "queue.h"

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {

  int size = 1;
  queue_t *ptr;

  if ( !queue )
    return 0;

  ptr = queue->next;
  
  while ( ptr != queue ) {
    ptr = ptr->next;
    size++;
  }

  return size;

}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {

  queue_t *ptr = queue;

  fprintf(stdout, "%s", name);
  fprintf(stdout, ": [");

  if ( ptr ) {
    print_elem(ptr);
    ptr = ptr->next;

    while ( ptr != queue ) {
      fprintf(stdout, " ");
      print_elem(ptr);
      ptr = ptr->next;
    }
  }

  fprintf(stdout, "]\n");

}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) {

  // verifica se a fila existe
  if ( !queue ) {
    fprintf(stderr, "ERRO: tentou inserir em fila inexistente\n");
    return -1;
  }
  // verifica se elemento existe
  if ( !elem ) {
    fprintf(stderr, "ERRO: tentou inserir elemento inexistente\n");
    return -1;
  }
  // verifica se elemento nao esta em outra fila
  if ( elem->prev || elem->next ) {
    fprintf(stderr, "ERRO: tentou inserir elemento existente em outra fila\n");
    return -1;
  }

  // se a fila está vazia
  if ( !(*queue) ) {
    (*queue) = elem;
    (*queue)->prev = elem;
    (*queue)->next = elem;
  } else {
    // apontando o último elemento atual como anterior do novo elemento
    elem->prev = (*queue)->prev;
    // apontando novo elemento como posterior ao último elemento atual
    (*queue)->prev->next = elem;
    // apontando o primeiro elemento como posterior do novo elemento
    elem->next = (*queue);
    // apontando novo elemento como anterior ao primeiro elemento
    (*queue)->prev = elem;
  }  

  return 0;

}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem) {
  // verifica se a fila existe
  if ( !queue ) {
    fprintf(stderr, "ERRO: tentou remover em fila inexistente\n");
    return -1;
  }
  // verifica se a fila está vazia
  if ( !(*queue) ) {
    fprintf(stderr, "ERRO: tentou remover em fila vazia\n");
    return -1;
  }
  // verifica se elemento existe
  if ( !elem ) {
    fprintf(stderr, "ERRO: tentou remover elemento inexistente\n");
    return -1;
  }
  // verifica se elemento nao tem fila
  if ( !elem->prev || !elem->next ) {
    fprintf(stderr, "ERRO: tentou remover elemento que não está em nenhuma fila\n");
    return -1;
  }
  // verifica se elemento pertence a fila indicada
  queue_t *ptr = (*queue)->next;
  while ( ptr != elem && ptr != (*queue) )
    ptr = ptr->next;

  if ( ptr != elem ) {
    fprintf(stderr, "ERRO: tentou remover elemento não pertencente a fila\n");
    return -1;
  }
  
  // se a fila tiver apenas o elemento a ser excluído
  if ( elem->next == elem && elem->prev == elem ) {
    (*queue) = NULL;
  } else { 
    // se o elemento está no inicio da fila
    if ( elem == (*queue) )
      (*queue) = elem->next;

    // ajustando apontadores
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
  }
  
  elem->next = NULL;
  elem->prev = NULL;

  return 0;
}