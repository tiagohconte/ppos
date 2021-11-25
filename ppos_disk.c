// PingPongOS - PingPong Operating System
// GRR20190374 - Tiago Henrique Conte, DINF UFPR
// Novembro de 2021

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "ppos.h"
#include "disk.h"
#include "ppos_disk.h"

// variáveis globais e constantes ==============================================

extern task_t *currentTask;
extern int userTasks;
task_t taskDiskMgr;
semaphore_t diskSleep;
disk_t disk;
int diskSignal, signalLock;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction diskAction;

// funções locais ==============================================================

// funcoes para operacoes atomicas
extern void enter_cs (int *lock);
extern void leave_cs (int *lock);

// tratador de sinal de disco
static void disk_handler (int signum) {
  // acorda tarefa de gerente de disco, caso esteja dormindo
  if (taskDiskMgr.status == 3) 
    sem_up(&(diskSleep));

  enter_cs(&signalLock);
  diskSignal = 1;
  leave_cs(&signalLock);
}

/*!
  \brief Tarefa gerente de disco
*/
static void disk_manager () {

  request_t *req = NULL;

  while (1) {
    
    sem_down(&(disk.access));

    if ( diskSignal ) {
      enter_cs(&signalLock);
      diskSignal = 0;
      leave_cs(&signalLock);
      req = disk.queue;
      sem_up(&(req->wait));
      if ( queue_remove( (queue_t **) &(disk.queue), (queue_t *) req ) )
        fprintf(stderr, "[PPOS error] disk_manager: fail to remove request from queue\n");
    }

    // verifica se o disco esta livre e se existem pedidos a serem atendidos
    if ( (disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) && disk.queue ) {
      req = disk.queue;
      
      // verifica se a operacao desejada eh leitura ou escrita
      switch ( req->type ) {
      case READ_OPERATION:
        if ( disk_cmd(DISK_CMD_READ, req->block, req->buffer) ) {
          fprintf(stderr, "[PPOS error] disk_manager: fail to read disk\n");
          req->exit_code = -1;
        }
        break;
      case WRITE_OPERATION:
        if ( disk_cmd(DISK_CMD_WRITE, req->block, req->buffer) ) {
          fprintf(stderr, "[PPOS error] disk_manager: fail to write disk\n");
          req->exit_code = -1;
        }
        break;
      default:
        fprintf(stderr, "[PPOS error] disk_manager: wrong disk operation\n");
        req->exit_code = -1;
        break;
      }
    }

    sem_up(&(disk.access));

    sem_down(&(diskSleep));

  }

}

// funções gerais ==============================================================

/*!
  \brief Inicializacao do gerente de disco

  \param numBlocks Tamanho do disco, em blocos
  \param blockSize Tamanho de cada bloco do disco, em bytes

  \return -1 em erro ou 0 em sucesso
*/  
int disk_mgr_init (int *numBlocks, int *blockSize) {

  // inicializa disco
  if ( disk_cmd(DISK_CMD_INIT, 0, 0) ) {
    fprintf(stderr, "[PPOS error] disk_mgr_init: fail to initialize disk\n");
    return(-1);
  }

  // coleta a quantidade de blocos e o tamanho dos blocos
  disk.numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
  if ( disk.numBlocks < 0 ) {
    fprintf(stderr, "[PPOS error] disk_mgr_init: fail to collect disk size\n");
    return(-1);
  }
  disk.blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
  if ( disk.blockSize < 0 ) {
    fprintf(stderr, "[PPOS error] disk_mgr_init: fail to collect block size\n");
    return(-1);
  }
  disk.queue = NULL;

  *numBlocks = disk.numBlocks;
  *blockSize = disk.blockSize;

  if ( sem_create(&(disk.access), 1) )
    return(-1);
  if ( sem_create(&(diskSleep), 0) )
    return(-1);

  diskSignal = 0;

  // registra a ação para o sinal de timer SIGALRM
  diskAction.sa_handler = disk_handler ;
  sigemptyset (&diskAction.sa_mask) ;
  diskAction.sa_flags = 0 ;
  if ( sigaction( SIGUSR1, &diskAction, 0 ) < 0 ) {
    perror ("Erro em sigaction disk: ") ;
    exit(-1) ;
  }

  // cria tarefa de gerenciador de disco
  task_create(&taskDiskMgr, disk_manager, NULL);
  userTasks--;

  return(0);
}

/*!
  \brief Leitura de um bloco, do disco para o buffer

  \return -1 em erro ou 0 em sucesso
*/  
int disk_block_read (int block, void *buffer) {
  // preenche struct de pedido
  request_t req;  
  req.prev = NULL;
  req.next = NULL;
  req.block = block;
  req.buffer = buffer;
  req.task = currentTask;
  req.type = READ_OPERATION;
  sem_create(&(req.wait), 0);
  req.exit_code = 0;
  
  if ( sem_down(&(disk.access)) )
    return(-1);

  // insere pedido na fila do disco
  if ( queue_append( (queue_t **) &(disk.queue), (queue_t *) &req) ) {
    sem_up(&(disk.access));
    fprintf(stderr, "[PPOS error] disk_block_read: fail to append request on queue\n");
    return(-1);
  }
    
  // se tarefa gerente de disco estah dormindo, acorda
  if ( taskDiskMgr.status == 3 ) {
    sem_up(&(diskSleep));
  }
  
  if ( sem_up(&(disk.access)) )
    return(-1);

  // espera o disco terminar a operacao
  sem_down(&(req.wait));

  sem_destroy(&(req.wait));

  return(req.exit_code);
}

/*!
  \brief Escrita de um bloco, do buffer para o disco

  \return -1 em erro ou 0 em sucesso
*/  
int disk_block_write (int block, void *buffer) {
  // preenche struct de pedido
  request_t req;  
  req.prev = NULL;
  req.next = NULL;
  req.block = block;
  req.buffer = buffer;
  req.task = currentTask;
  req.type = WRITE_OPERATION;
  sem_create(&(req.wait), 0);
  req.exit_code = 0;
  
  if ( sem_down(&(disk.access)) )
    return(-1);

  // insere pedido na fila do disco
  if ( queue_append( (queue_t **) &(disk.queue), (queue_t *) &req) ) {
    sem_up(&(disk.access));
    fprintf(stderr, "[PPOS error] disk_block_write: fail to append request on queue\n");
    return(-1);
  }
    
  // se tarefa gerente de disco estah dormindo, acorda
  if ( taskDiskMgr.status == 3 ) {
    sem_up(&(diskSleep));
  }
  
  if ( sem_up(&(disk.access)) )
    return(-1);

  // espera o disco terminar a operacao
  sem_down(&(req.wait));

  sem_destroy(&(req.wait));

  return(req.exit_code);
}