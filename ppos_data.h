// PingPongOS - PingPong Operating System
// Tiago Henrique Conte, DINF UFPR
// Novembro de 2021

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   int status ;   // status da tarefa ( 1 = PRONTA, 2 = TERMINADA, 3 = SUSPENSA )
   int est_prio ;  // prioridade estática da tarefa ( de -20 à +20, sendo -20 maior prioridade)
   int din_prio ;  // prioridade dinâmica da tarefa
   int system_task ;  // task de sistema? ( 0 = NÃO, 1 = SIM)
   unsigned int inic_time;  // tempo em que a tarefa foi iniciada
   unsigned int proc_time;  // tempo de processamento da tarefa
   unsigned int wake_time;  // tempo em que a tarefa deve ser acordada
   unsigned int activ;  // quantidade de ativações do processo
   unsigned int exit_code;  // exit code da tarefa
   struct task_t *joinedQueue;  // fila de tarefas esperando fim da task
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

