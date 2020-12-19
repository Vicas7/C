#ifndef __CLINIC_IUL_H__
#define __CLINIC_IUL_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>

#define exit_on_error(s,m) if (s<0) {perror(m);exit(1);}
#define exit_on_null(s,m) if (s==NULL) {perror(m);exit(1);}

#define IPC_KEY 0x0a93918
#define DURACAO 10

typedef struct {
  int tipo; // Tipo de Consulta: 1-Normal, 2-COVID19, 3-Urgente
  char descricao[100]; // Descrição da Consulta
  int pid_consulta; // PID do processo que quer fazer a consulta
  int status; // Estado da consulta: 1-Pedido, 2-Iniciada,
} Consulta; // 3-Terminada, 4-Recusada, 5-Cancelada

typedef struct {
  long tipo;
  Consulta consulta;
} Mensagem;

typedef struct {
  Consulta lista_consultas[10];
  int contadores[4]; // 0- Normal, 1- Covid-19, 2- Urgente, 3- Perdidas
} Memoria;


#endif