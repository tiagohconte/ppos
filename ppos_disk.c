// PingPongOS - PingPong Operating System
// GRR20190374 - Tiago Henrique Conte, DINF UFPR
// Novembro de 2021

#include <stdlib.h>
#include <stdio.h>
#include "ppos_disk.h"
#include "disk.h"

// variáveis globais e constantes ==============================================


// funções locais ==============================================================


// funções gerais ==============================================================

/*!
  \brief Inicializacao do gerente de disco

  \param numBlocks Tamanho do disco, em blocos
  \param blockSize Tamanho de cada bloco do disco, em bytes

  \return -1 em erro ou 0 em sucesso
*/  
int disk_mgr_init (int *numBlocks, int *blockSize) {

}

/*!
  \brief Leitura de um bloco, do disco para o buffer

  \return -1 em erro ou 0 em sucesso
*/  
int disk_block_read (int block, void *buffer) {

}

/*!
  \brief Escrita de um bloco, do buffer para o disco

  \return -1 em erro ou 0 em sucesso
*/  
int disk_block_write (int block, void *buffer) {

}