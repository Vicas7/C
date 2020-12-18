#include "clinic-iul.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

int n = 0;
int alarms = 0;

void SIGHUP_handler(int sinal) {
  printf("Consulta iniciada para o processo <%d>\n", getpid());
  remove("PedidoConsulta.txt");
  n = 1;
}

void SIGTERM_handler(int sinal) {
  if (n == 1) {
    printf("Consulta concluída para o processo <%d>\n", getpid());
    exit(0);
  }
  else {
    fprintf(stderr, "Erro! Recebeu SIGTERM antes de SIGHUP");
    exit(1);
  }
}

void SIGUSR2_handler(int sinal) {
  printf("Consulta não é possível para o processo <%d>\n", getpid());
  remove("PedidoConsulta.txt");
  exit(0);
}

int get_srv_pid() {
  FILE* f;
  int srvpid;
  f = fopen("SrvConsultas.pid", "r");
  if (!f) {
    fprintf(stderr, "Erro abertura do ficheiro SrvConsultas.pid");
    exit(1);
  }
  fscanf(f, "%d", &srvpid);
  fclose(f);
  return srvpid;
}

void escrita_pedido_consulta(Consulta c) {
  FILE* f;
  f = fopen("PedidoConsulta.txt", "w");
  if (!f) {
    fprintf(stderr, "Erro abertura do ficheiro PedidoConsulta.txt");
    exit(1);
  }
  fprintf(f, "%d,%s,%d", c.tipo, c.descricao, c.pid_consulta);
  fclose(f);
}

Consulta criar_cliente() {
  int tipo;
  char descricao[100];

  // Input do cliente
  printf("Cliente Clinic-IUL [%d]\n", getpid());
  printf("Nova consulta\n");
  printf("Tipo de Consulta (1-Normal, 2-COVID19, 3-Urgente): ");
  scanf("%d%*c", &tipo);

  if (tipo < 1 || tipo > 3) {
    fprintf(stderr, "Erro! Escolha um tipo de consulta entre 1 e 3");
    exit(1);
  }
  printf("Descrição: ");
  fgets(descricao, 100, stdin);
  descricao[strlen(descricao) - 1] = '\0';

  //Criar consulta
  printf("A criar consulta...\n");
  Consulta c;
  c.tipo = tipo;
  strncpy(c.descricao, descricao, 100);
  c.pid_consulta = getpid();

  return c;
}

void SIGINT_handler(int sinal) {
  printf("\nPaciente cancelou o pedido\n");
  remove("PedidoConsulta.txt");
  exit(1);
}

void SIGALRM_handler(int sinal) {
  alarms = 1;
}

void verificar_enviar_sinal(Consulta c) {
  if (access("PedidoConsulta.txt", F_OK) == 0) {
    fprintf(stderr, "Erro: Já existe um pedido de consulta. Tentaremos automáticamente daqui 10 segundos\n");
    signal(SIGALRM, SIGALRM_handler);
    alarm(10);
    while (alarms == 0)
      pause();
    alarms = 0;
    verificar_enviar_sinal(c);
  }
  else {
    escrita_pedido_consulta(c);
    kill(get_srv_pid(), SIGUSR1);
  }
}

int main() {
  // Receber sinal sigint
  signal(SIGINT, SIGINT_handler);

  Consulta c = criar_cliente();
  verificar_enviar_sinal(c);

  // Envio de sinal para o servidor

  // Receber sinais do servidor
  signal(SIGUSR2, SIGUSR2_handler);
  signal(SIGHUP, SIGHUP_handler);
  signal(SIGTERM, SIGTERM_handler);
  char s[100];
  while (1) {
    fgets(s, 100, stdin);
    s[strlen(s) - 1] = 0;
    if (strcmp(s, "sair") == 0)
      exit(0);
  }

  return 0;
}