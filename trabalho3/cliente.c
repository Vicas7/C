#include "clinic-iul.h"

Consulta obter_consulta() {
  int tipo;
  char descricao[100];
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

  Consulta c;
  c.pid_consulta = getpid();
  c.status = 1;
  c.tipo = tipo;
  strncpy(c.descricao, descricao, 100);

  return c;
}

void cancela_consulta() {
  int msg_id = msgget(IPC_KEY, 0);
  exit_on_error(msg_id, "[cliente] Erro a ligar a msg");

  Mensagem m;
  m.tipo = getpid();
  m.consulta.tipo = 5;

  int status;
  status = msgsnd(msg_id, &m, sizeof(m.consulta), 0);
  exit_on_error(status, "erro ao enviar");
}

void SIGINT_handler(int sinal) {
  printf("Paciente cancelou o pedido\n");
  cancela_consulta();
  exit(1);
}

int main() {
  // Receber sinal sigint
  signal(SIGINT, SIGINT_handler);

  Consulta c = obter_consulta();

  int msg_id = msgget(IPC_KEY, 0);
  exit_on_error(msg_id, "[cliente] Erro a ligar a msg");

  Mensagem m;
  m.tipo = 1;
  m.consulta = c;

  int status;
  status = msgsnd(msg_id, &m, sizeof(m.consulta), 0);
  exit_on_error(status, "erro ao enviar");

  int comecou = 0;
  while (1) {
    status = msgrcv(msg_id, &m, sizeof(m.consulta), m.consulta.pid_consulta, 0);
    exit_on_error(status, "erro ao receber");

    if (m.consulta.status == 2) {
      printf("[Cliente] Consulta iniciada para o processo %d\n", m.consulta.pid_consulta);
      comecou = 1;
    }
    if (m.consulta.status == 3) {
      if (comecou) {
        printf("[Cliente] Consulta concluída para o processo %d\n", m.consulta.pid_consulta);
        exit(0);
      }
      else {
        comecou = -1;
        exit_on_error(comecou, "Tentativa de conclusão antes de ser iniciada");
      }
    }
    if (m.consulta.status == 4) {
      printf("[Cliente] Consulta não é possível para o processo %d\n", m.consulta.pid_consulta);
      exit(0);
    }
  }
}