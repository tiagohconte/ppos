// PingPongOS - PingPong Operating System
// Tiago Henrique Conte, DINF UFPR
// Versão 0.1 - Outubro de 2021

#include <stdlib.h>
#include <stdio.h>
#include "ppos.h"

// variáveis globais e constantes ==============================================

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t taskMain, *taskAtual, *filaTask = NULL;
unsigned int taskCount = 0;

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf ( stdout, 0, _IONBF, 0 ) ;

  taskMain.id = taskCount++;
  taskMain.prev = NULL;
  taskMain.next = NULL;
  getcontext( &(taskMain.context) );

  if ( queue_append ((queue_t **) &filaTask, (queue_t*) &taskMain) ) {
    fprintf(stderr, "Erro ao adicionar a tarefa à fila.\n");
  }

  taskAtual = &taskMain;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: sistema iniciado\n");
  #endif

}

// gerência de tarefas =========================================================

/*!
  \brief Cria uma nova tarefa

  \param task Estrutura que referencia a tarefa criada
  \param start_func Função que será executada pela tarefa
  \param arg Parâmetro a passar para a tarefa que está sendo criada

  \return ID (>0) da tarefa criada ou um valor negativo, se houver erro
*/
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

  char *stack;

  task->id = taskCount++;
  task->prev = NULL;
  task->next = NULL;
  getcontext( &(task->context) );

  // aloca stack
  stack = malloc (STACKSIZE);
  if (stack) {
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
  } else {
    perror ("Erro na criação da pilha: ");
    return (-1);
  }  

  // cria contexto com a função passada
  makecontext ( &(task->context), (void*)(start_func), 1, arg );

  // adiciona task a fila de tasks
  if ( queue_append ((queue_t **) &filaTask, (queue_t*) task) )
    return -1;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d criada por task %d\n", task->id, taskAtual->id);
  #endif

  return task->id;

}

/*!
  \brief Termina a tarefa corrente

  \param exitCode código de término devolvido pela tarefa corrente
*/
void task_exit (int exitCode) {

  // remove task da fila de tasks
  queue_remove ((queue_t**) &filaTask, (queue_t*) taskAtual);

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d exited\n", taskAtual->id);
  #endif

  task_switch( &taskMain );

}

/*!
  \brief Alterna a execução para a tarefa indicada

  \param task Tarefa que irá assumir o processador

  \return Zero em sucesso, valor negativo se houver erro
*/
int task_switch (task_t *task) {

  task_t *aux;

  if (!task){
    fprintf(stderr, "ERRO: switch para task inexistente");
    return -1;
  }

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: switch task %d -> task %d\n", taskAtual->id, task->id);
  #endif

  aux = taskAtual;
  taskAtual = task;

  swapcontext ( &(aux->context), &(task->context) );

  return 0;

}

/*!
  \brief Retorna o identificador da tarefa corrente

  \return Identificador numérico (ID) da tarefa corrente
*/
int task_id () {
  return taskAtual->id;
}