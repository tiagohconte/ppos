// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#define READ_OPERATION 0
#define WRITE_OPERATION 1

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um request de disco
typedef struct
{
  struct request_t *prev, *next; // ponteiros para usar em filas
  task_t *task;     // task pedindo o disco
  int type;         // READ_OPERATION ou WRITE_OPERATION
  int block;        // bloco da operacao
  void *buffer;     // buffer de dados
  int exit_code;    // exit_code da tarefa
  semaphore_t wait; // tarefa aguarda disco
} request_t ;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  request_t *queue;   // fila de pedidos de disco
  semaphore_t access; // semaforo de acesso ao disco
  int numBlocks;      // quantidade de blocos no disco
  int blockSize;      // tamanho do bloco do disco
} disk_t ;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif
