#include "clinic-iul.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

Consulta lista_consultas[10];
int n = 0;
int tipo1 = 0;
int tipo2 = 0;
int tipo3 = 0;
int perdidas = 0;
int alarms = 0;

void escreverPid() {
  FILE* f;
  int srvpid;
  f = fopen("SrvConsultas.pid", "w");
  if (!f) {
    fprintf(stderr, "Erro abertura do ficheiro\n");
    exit(1);
  }
  fprintf(f, "%d\n", getpid());
  fclose(f);
}

void SIGINT_handler(int sinal) {
  remove("SrvConsultas.pid");
  FILE* f = fopen("StatsConsultas.dat", "wb");
  if (!f) {
    fprintf(stderr, "Erro abertura do ficheiro\n");
    exit(1);
  }
  fwrite(&perdidas, sizeof(perdidas), 1, f);
  fwrite(&tipo1, sizeof(tipo1), 1, f);
  fwrite(&tipo2, sizeof(tipo2), 1, f);
  fwrite(&tipo3, sizeof(tipo3), 1, f);
  fclose(f);
  exit(0);
}

void SIGALARM_handler(int sinal) {
  alarms = 1;
}

void SIGUSR1_handler(int sinal) {
  FILE* f;
  f = fopen("PedidoConsulta.txt", "r");

  if (!f) {
    fprintf(stderr, "Erro na leitura do ficheiro 'PedidoConsulta.txt\n");
    fclose(f);
    exit(1);
  }
  char buffer[256];
  int tipo;
  char descricao[100];
  int pid;
  fscanf(f, "%d,%100[^,]%*c%d", &tipo, descricao, &pid);
  printf("Chegou novo pedido de consulta do tipo < %d >, descrição < %s > e PID < %d >\n", tipo, descricao, pid);

  if (n == 10) {
    printf("Lista de consultas cheia\n");
    kill(pid, SIGUSR2);
    perdidas++;
  }
  else {
    int i;
    for (i = 0; i != 10; i++) {
      if (lista_consultas[i].tipo == -1) {
        lista_consultas[i].tipo = tipo;
        strncpy(lista_consultas[i].descricao, descricao, 100);
        lista_consultas[i].pid_consulta = pid;

        printf("Consulta agendada para a sala %d\n", i);
        n++;
        switch (tipo) {
        case 1: tipo1++;break;
        case 2: tipo2++;break;
        case 3: tipo3++;break;
        }
        break;
      }
    }

    int pidFilho = fork();

    if (pidFilho == -1) {
      fprintf(stderr, "Erro na criação do filho para a consulta com o pid %d\n", pid);
    }
    else if (pidFilho == 0) {
      kill(pid, SIGHUP);
      signal(SIGALRM, SIGALARM_handler);
      alarm(10);
      while (alarms == 0)
        pause();
      printf("Consulta terminada na sala %d\n", i);
      kill(pid, SIGTERM);
      exit(0);
    }
    else {
      wait(NULL);
      lista_consultas[i].tipo = -1;
    }
  }
}


int main() {
  for (int i = 0; i != 10; i++)
    lista_consultas[i].tipo = -1;

  printf("Bem vindo ao servidor da Clinic-IUL [%d]\n", getpid());
  escreverPid();

  signal(SIGUSR1, SIGUSR1_handler);
  signal(SIGINT, SIGINT_handler);

  while (1) {
    pause();
  }
  return 0;
}