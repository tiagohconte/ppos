// PingPongOS - PingPong Operating System
// Tiago Henrique Conte, DINF UFPR
// Versão 0.1 - Outubro de 2021

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"

// variáveis globais e constantes ==============================================

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define TASK_AGING -1

task_t taskMain, taskDispatcher, *currentTask, *lastTask;
task_t *readyQueue = NULL;
unsigned int taskCount = 0, userTasks = 0, quantum_count, ticks;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction ticksAction ;

// estrutura de inicialização to timer
struct itimerval timer ;

// funções locais ==============================================================

/*!
  \brief Função para impressão de fila
*/  
void print_elem (void *ptr) {

  task_t *elem = ptr ;

  if (!elem)
    return ;

  printf ("%d", elem->id) ;

}
void print_prio (void *ptr) {

  task_t *elem = ptr ;

  if (!elem)
    return ;

  printf ("(%d -> %d)", elem->id, elem->din_prio) ;

}

/*!
  \brief Escalonador de tarefas

  \return Retorna o endereço da próxima task
*/  
static task_t * scheduler () {

  task_t *nextTask = readyQueue, *task;

  task = nextTask->next;

  #ifdef DEBUG
  queue_print("Dynamic Priority ", (queue_t *) readyQueue, print_prio);
  #endif

  // busca a próxima tarefa e realiza aging de tarefas
  do {
    // verifica se a prioridade dinâmica da task analisada é maior/igual do que a definida atualmente
    if ( task->din_prio == nextTask->din_prio ) {
      // caso prio. dinamicas sejam iguais, verifica a prioridade estática
      if ( task->est_prio < nextTask->est_prio ) {
        nextTask->din_prio+=TASK_AGING;
        nextTask = task;
      }
    } else if ( task->din_prio < nextTask->din_prio ) {
      // caso a proxima seja maior, a definida anteriormente sofre aging
      nextTask->din_prio+=TASK_AGING;
      nextTask = task;
    } else {
      // caso a prioridade não seja maior, a tarefa sofre aging
      task->din_prio+=TASK_AGING;
    }
    task = task->next;  // incremento
  } while ( task != readyQueue ); // verifica até voltar ao início

  // reseta a prioridade dinâmica da tarefa que será executada em seguida
  nextTask->din_prio = nextTask->est_prio;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task scheduler: task %d is the next\n", nextTask->id);
  #endif

  return ( nextTask );

}
/*!
  \brief Despachante de tarefas
*/  
static void dispatcher () {

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task dispatcher launched\n");
  #endif

  task_t *nextTask;
  unsigned int proc_time;

  // enquanto houverem user tasks
  while ( userTasks > 0 ) {
    
    // escolhe a próxima tarefa a ser executada
    nextTask = scheduler();

    // se escalonador escolheu tarefa
    if ( nextTask ) {
      // seta quantum counter
      quantum_count = 20;
      // coleta tempo de sistema antes de entrar na tarefa
      proc_time = systime();
      // switch para prox tarefa
      task_switch(nextTask);
      // calcula o tempo de processamento da tarefa
      lastTask->proc_time += systime() - proc_time;

      // voltando ao dispatcher, trata a tarefa conforme seu estado
      /*  1 = PRONTA
          2 = TERMINADA
          3 = SUSPENSA  */
      switch ( lastTask->status ) {
      case 1:    
        // adiciona task ao fim da fila de tasks prontas
        if ( queue_append ((queue_t **) &readyQueue, (queue_t*) lastTask) ) {
          fprintf(stderr, "[PPOS error]: dispatcher: fail adding task to queue\n");
          exit(-1);
        }    
        break;
      case 2:
        free(lastTask->context.uc_stack.ss_sp);
        lastTask->context.uc_stack.ss_size = 0;
        // remove task da fila de tasks prontas
        if ( queue_remove ((queue_t**) &readyQueue, (queue_t*) lastTask) ) {
          fprintf(stderr, "[PPOS error]: dispatcher: fail removing task from queue\n");
          exit(-1);
        }
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

  free(currentTask->context.uc_stack.ss_sp);
  currentTask->context.uc_stack.ss_size = 0;

  task_exit(0);

  return;
}

// tratador de sinal de ticks de relógio
static void ticks_handler (int signum) {
  // incrementa contador de ticks e tempo de processamento da tarefa atual
  ticks++;

  // se não é tarefa de sistema, decrementa quantum
  if ( !( currentTask->system_task ) ) {
    quantum_count--;
    // quando o contador chega em zero, devolve CPU
    if ( quantum_count == 0 )
      task_yield();
  }

}

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf ( stdout, 0, _IONBF, 0 ) ;

  ticks = 0;

  taskMain.id = taskCount++;
  taskMain.prev = NULL;
  taskMain.next = NULL;
  taskMain.status = 1;
  taskMain.system_task = 0;
  taskMain.est_prio = 0;
  taskMain.din_prio = 0;
  taskMain.inic_time = systime();
  taskMain.proc_time = 0;
  taskMain.activ = 0;
  taskMain.exit_code = 0;
  taskMain.joinedQueue = NULL;
  getcontext( &(taskMain.context) );

  userTasks++;

  if ( queue_append ((queue_t **) &readyQueue, (queue_t*) &taskMain) ) {
    fprintf(stderr, "[PPOS error]: adding main task to readyQueue.\n");
    exit(-1);
  }

  currentTask = &taskMain;

  // cria o dispatcher e remove ele da fila
  if ( task_create(&taskDispatcher, dispatcher, NULL) < 0 ) {
    fprintf(stderr, "[PPOS error]: creating dispatcher\n");
    exit(-1);
  }
  if ( queue_remove ((queue_t**) &readyQueue, (queue_t*) &taskDispatcher) ) {
    fprintf(stderr, "[PPOS error]: removing dispatcher task from queue\n");
    exit(-1);
  }

  taskDispatcher.system_task = 1;
  userTasks--;

  // registra a ação para o sinal de timer SIGALRM
  ticksAction.sa_handler = ticks_handler ;
  sigemptyset (&ticksAction.sa_mask) ;
  ticksAction.sa_flags = 0 ;
  if ( sigaction( SIGALRM, &ticksAction, 0 ) < 0 ) {
    perror ("Erro em sigaction ticks: ") ;
    exit(-1) ;
  }

  // ajuste de valores do temporizador
  timer.it_value.tv_usec = 1000 ;       // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0 ;          // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000 ;    // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0 ;       // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
    perror ("Erro em setitimer: ") ;
    exit(-1) ;
  }

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: system initialized\n");
  #endif

  task_switch(&taskDispatcher);

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
  task->est_prio = 0;
  task->din_prio = 0;
  task->system_task = 0;
  task->inic_time = systime();
  task->proc_time = 0;
  task->activ = 0;
  task->exit_code = 0;
  task->joinedQueue = NULL;
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
  currentTask->exit_code = exitCode;

  #ifdef DEBUG
  queue_print("Joined queue ", (queue_t *) currentTask->joinedQueue, print_elem);
  #endif

  task_t *task = currentTask->joinedQueue;
  while (task) {
    // remove task da fila de joined tasks
    if ( queue_remove ((queue_t**) &(currentTask->joinedQueue), (queue_t*) task) ) {
      fprintf(stderr, "[PPOS error]: task exit: fail removing task from joined queue\n");
      exit(-1);
    }

    task->status = 1;

    // adiciona task a fila de tasks prontas
    if ( queue_append ((queue_t **) &readyQueue, (queue_t*) task) ) {
      fprintf(stderr, "[PPOS error]: task exit: fail adding task to ready queue\n");
      exit(-1);
    }
    userTasks++;

    task = currentTask->joinedQueue;
  }

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d exited\n", currentTask->id);
  #endif

  fprintf(stdout, "Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currentTask->id, (systime() - currentTask->inic_time), currentTask->proc_time, currentTask->activ );

  if ( currentTask == &taskDispatcher )
    exit(0);
  
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

  task->activ++;

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

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d yields the CPU\n", currentTask->id);
  #endif

  task_switch(&taskDispatcher);
}

/*!
  \brief Define a prioridade estática de uma tarefa (ou a tarefa atual)
*/
void task_setprio (task_t *task, int prio) {
  if ( !task )
    task = currentTask;

  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: task %d priority setted to %d \n", task->id, prio);
  #endif

  task->est_prio = prio;
  task->din_prio = prio; 
  
}

/*!
  \brief Retorna a prioridade estática de uma tarefa (ou a tarefa atual)
*/
int task_getprio (task_t *task) {
  if ( !task )
    task = currentTask;
  
  #ifdef DEBUG
  fprintf(stdout, "[PPOS debug]: got priority of task %d\n", task->id);
  #endif

  return ( task->est_prio );  

}

// operações de sincronização ==================================================

/*! 
  \brief a tarefa corrente aguarda o encerramento de outra task
*/
int task_join (task_t *task) {
  // verifica se a tarefa existe e não foi exited
  if ( !task || task->status == 2 )
    return (-1);

  // remove task da fila de tasks prontas
  if ( queue_remove ((queue_t**) &readyQueue, (queue_t*) currentTask) ) {
    fprintf(stderr, "[PPOS error]: task join: fail removing task from ready queue\n");
    exit(-1);
  }
  userTasks--;

  // seta o status da tarefa atual para suspensa
  currentTask->status = 3;
  
  // adiciona task atual a fila de tasks join da task passada via parametro
  if ( queue_append ((queue_t **) &(task->joinedQueue), (queue_t*) currentTask) ) {
    fprintf(stderr, "[PPOS error]: task join: fail adding task to joined queue\n");
    exit(-1);
  }

  #ifdef DEBUG
  fprintf(stderr, "[PPOS debug]: added to queue\n");
  queue_print("Ready queue ", (queue_t *) readyQueue, print_elem);
  queue_print("Joined queue ", (queue_t *) task->joinedQueue, print_elem);
  #endif

  task_switch(&taskDispatcher);

  return task->exit_code;

}

// operações de gestão do tempo ================================================

/*!
  \brief Retorna o relógio atual (em milisegundos)
*/
unsigned int systime () {
  return ticks;
}