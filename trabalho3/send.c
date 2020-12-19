#include "clinic-iul.h"
Memoria memoria;

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


  int shm_id = shmget(IPC_KEY, sizeof(memoria), 0);
  exit_on_error(shm_id, "shmget");

  Consulta* c = (Consulta*)shmat(shm_id, NULL, 0);
  exit_on_null(c, "shmat");

  for (int i = 0; i < 10; i++) {
    printf("Consulta %d: Tipo - %d\n", i, c[i].tipo);
  }

  int sem_id = semget(IPC_KEY, 1, 0);
  exit_on_error(sem_id, "semget");

  int val = semctl(sem_id, 0, GETVAL);
  printf("O valor do semáforo é : %d\n", val);
}