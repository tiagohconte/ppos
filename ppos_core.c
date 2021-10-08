// PingPongOS - PingPong Operating System
// Tiago Henrique Conte, DINF UFPR
// Versão 0.1 - Outubro de 2021

#include <stdlib.h>
#include <stdio.h>
#include "ppos.h"

// variáveis globais e constantes ==============================================

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t taskMain, taskDispatcher, *currentTask, *lastTask;
task_t *readyQueue = NULL;
unsigned int taskCount = 0, userTasks = 0;

// funções locais ==============================================================

/*!
  \brief Função para impressão de fila
*/  
static void print_elem (void *ptr) {

  task_t *elem = ptr ;

  if (!elem)
    return ;

  printf ("%d", elem->id) ;

}

/*!
  \brief Escalonador de tarefas

  \return Retorna o endereço da próxima task
*/  
static task_t * scheduler () {

  return ( readyQueue );

}
/*!
  \brief Despachante de tarefas
*/  
static void dispatcher () {

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task dispatcher launched\n");
  queue_print("Ready ", (queue_t *) readyQueue, print_elem);
  #endif

  task_t *nextTask;

  // enquanto houverem user tasks
  while ( userTasks > 0 ) {
    
    // escolhe a próxima tarefa a ser executada
    nextTask = scheduler();

    // se escalonador escolheu tarefa
    if ( nextTask ) {
      // switch para prox tarefa
      task_switch(nextTask);

      // voltando ao dispatcher, trata a tarefa conforme seu estado
      /*  1 = PRONTA
          2 = TERMINADA
          3 = SUSPENSA  */
      switch ( lastTask->status ) {
      case 1:        
        break;
      case 2:
        free(lastTask->context.uc_stack.ss_sp);
        lastTask->context.uc_stack.ss_size = 0;
        // remove task da fila de tasks
        queue_remove ((queue_t**) &readyQueue, (queue_t*) lastTask);
        userTasks--;        
        break;
      case 3:
        break;
      default:
        break;
      }

      #ifdef DEBUG
      queue_print("Ready ", (queue_t *) readyQueue, print_elem);
      #endif

    }

  }

  task_exit(0);

  return;
}

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf ( stdout, 0, _IONBF, 0 ) ;

  taskMain.id = taskCount++;
  taskMain.prev = NULL;
  taskMain.next = NULL;
  taskMain.status = 1;
  getcontext( &(taskMain.context) );

  if ( queue_append ((queue_t **) &readyQueue, (queue_t*) &taskMain) ) {
    fprintf(stderr, "[PPOS error]: adding main task to readyQueue.\n");
    exit(-1);
  }

  currentTask = &taskMain;

  if ( task_create(&taskDispatcher, dispatcher, NULL) < 0 ) {
    fprintf(stderr, "[PPOS error]: creating dispatcher\n");
    exit(-1);
  }

  userTasks--;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: system initialized\n");
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
  task->status = 1;
  getcontext( &(task->context) );

  // aloca stack
  stack = malloc (STACKSIZE);
  if (stack) {
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
  } else {
    perror ("[PPOS error]: creating stack: ");
    return -1;
  }  

  // cria contexto com a função passada
  makecontext ( &(task->context), (void*)(start_func), 1, arg );

  // adiciona task a fila de tasks prontas
  if ( queue_append ((queue_t **) &readyQueue, (queue_t*) task) )
    return -1;
  
  userTasks++;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d created by task %d\n", task->id, currentTask->id);
  #endif

  return task->id;

}

/*!
  \brief Termina a tarefa corrente

  \param exitCode código de término devolvido pela tarefa corrente
*/
void task_exit (int exitCode) {

  currentTask->status = 2;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d exited\n", currentTask->id);
  #endif

  if ( currentTask == &taskDispatcher )
    task_switch( &taskMain );
  else
    task_switch( &taskDispatcher );

}

/*!
  \brief Alterna a execução para a tarefa indicada

  \param task Tarefa que irá assumir o processador

  \return Zero em sucesso, valor negativo se houver erro
*/
int task_switch (task_t *task) {

  if (!task){
    fprintf(stderr, "[PPOS error]: switch to inexistent task");
    return -1;
  }

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: switch task %d -> task %d\n", currentTask->id, task->id);
  #endif

  lastTask = currentTask;
  currentTask = task;

  swapcontext ( &(lastTask->context), &(task->context) );

  return 0;

}

/*!
  \brief Retorna o identificador da tarefa corrente

  \return Identificador numérico (ID) da tarefa corrente
*/
int task_id () {
  return currentTask->id;
}

// operações de escalonamento ==================================================

/*!
  \brief Libera o processador para a próxima tarefa
*/
void task_yield () {
  if ( queue_remove ((queue_t**) &readyQueue, (queue_t*) currentTask) ) {
    fprintf(stderr, "[PPOS error]: removing current task from queue\n");
    exit(-1);
  }

  if ( !( currentTask == &taskMain ) ) {
    // adiciona task ao fim da fila de tasks prontas
    if ( queue_append ((queue_t **) &readyQueue, (queue_t*) currentTask) ) {
      fprintf(stderr, "[PPOS error]: adding task to queue\n");
      exit(-1);
    }
  } else {
    if ( queue_remove ((queue_t**) &readyQueue, (queue_t*) &taskDispatcher) ) {
      fprintf(stderr, "[PPOS error]: removing dispatcher task from queue\n");
      exit(-1);
    }
  }

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d yields the CPU\n", currentTask->id);
  #endif

  task_switch(&taskDispatcher);
}