#include "clinic-iul.h"

int main() {
  // int id = msgget(IPC_KEY, 0);
  // exit_on_error(id, "[cliente] Erro a ligar");

  // Mensagem m;
  // m.tipo = 1;
  // m.consulta.tipo = 2;
  // m.consulta.status = 1;
  // m.consulta.pid_consulta = getpid();
  // printf("Mensagem : ");
  // fgets(m.consulta.descricao, 100, stdin);
  // m.consulta.descricao[strlen(m.consulta.descricao) - 1] = 0;

  // int status;
  // status = msgsnd(id, &m, sizeof(m.consulta), 0);
  // exit_on_error(status, "erro ao enviar");

  // printf("A mensagem foi enviada\n");

  // status = msgrcv(id, &m, sizeof(m.consulta), m.consulta.pid_consulta, 0);
  // exit_on_error(status, "erro ao receber");
  // printf("[send] A resposta foi do tipo %d com descrição : '%s'\n", m.consulta.status, m.consulta.descricao);

  // status = msgrcv(id, &m, sizeof(m.consulta), m.consulta.pid_consulta, 0);
  // exit_on_error(status, "erro ao receber");
  // printf("[send] A resposta foi do tipo %d com descrição : '%s'\n", m.consulta.status, m.consulta.descricao);
  // exit(0);


  int shm_id = shmget(IPC_KEY, 4 * sizeof(int) + 10 * sizeof(Consulta), 0);
  exit_on_error(shm_id, "shmget");

  Consulta* c = (Consulta*)shmat(shm_id, NULL, 0);
  exit_on_null(c, "shmat");

  for (int i = 0; i < 10; i++) {
    printf("Consulta %d: Tipo - %d\n", i, c[i].tipo);
  }
}