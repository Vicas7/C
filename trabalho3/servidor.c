#include "clinic-iul.h"
Memoria memoria;


// ! --------------------------------
// TODO: Remover comentários de debug 
// ! --------------------------------


Memoria* iniciar() {
  // Cria um semáforo para sincronização
  int sem_id = semget(IPC_KEY, 1, IPC_CREAT | 0666);
  exit_on_error(sem_id, "semget");

  // Inicializa o semáforo a 0 para impedir que outro processo possa
  // aceder à zona de memória partilhada antes da inicialização terminar
  int status = semctl(sem_id, 0, SETVAL, 0);
  exit_on_error(status, "semctl(SETVAL)");

  // Cria | conecta a zona de memória partilhada

  int mem_id = shmget(IPC_KEY, sizeof(memoria), 0);
  if (mem_id < 0)
    mem_id = shmget(IPC_KEY, sizeof(memoria), IPC_EXCL | IPC_CREAT | 0666);
  exit_on_error(mem_id, "shmget1");

  // Obtém o apontador para a zona de memória partilhada
  Memoria* mem = (Memoria*)shmat(mem_id, NULL, 0);
  exit_on_null(mem, "shmat");

  // Inicializa os valores
  for (int i = 0; i < 10; i++) {
    mem->lista_consultas[i].tipo = -1;
  }
  printf("Memória iniciada id=%d\n", mem_id);

  // A inicialização terminou, coloca o semáforo a 1 para que outros
  // processos possam aceder à zona de memória partilhada
  status = semctl(sem_id, 0, SETVAL, 1);
  exit_on_error(status, "semctl(SETVAL)");
  return mem;
}

void remover_consulta(int posicao, Memoria* mem) {
  mem->lista_consultas[posicao].tipo = -1;
}

void SIGINT_handler(int sinal) {
  printf("\nA encerrar servidor...\n\n");
  // Obter identificador da memória partilhada
  int mem_id = shmget(IPC_KEY, sizeof(memoria), 0);
  exit_on_error(mem_id, "shmget2");

  // Obter o apontador para a zona de memória partilhada
  Memoria* mem = (Memoria*)shmat(mem_id, NULL, 0);
  exit_on_null(mem, "shmat");

  printf("-------------- Estatísticas --------------\n");
  printf("| Normal | Covid-19 | Urgente | Perdidas |\n");
  printf("|   %d    |    %d     |    %d    |    %d     |\n", mem->contadores[0], mem->contadores[1], mem->contadores[2], mem->contadores[3]);
  printf("------------------------------------------\n");
  exit(1);
}

int main() {
  signal(SIGINT, SIGINT_handler);

  iniciar();
  // Obter identificador da memória partilhada
  int mem_id = shmget(IPC_KEY, sizeof(memoria), 0);
  exit_on_error(mem_id, "shmget3");

  // Obter o apontador para a zona de memória partilhada
  Memoria* mem = (Memoria*)shmat(mem_id, NULL, 0);
  exit_on_null(mem, "shmat");

  // Obter identificador do semáforo
  int sem_id = semget(IPC_KEY, 1, 0);
  exit_on_error(sem_id, "semget");

  // Especificar UP and DOWN
  struct sembuf UP = { .sem_op = +1 };
  struct sembuf DOWN = { .sem_op = -1 };

  // Obter identificador da mail list
  int msg_id = msgget(IPC_KEY, IPC_CREAT | IPC_EXCL | 0666);
  if (msg_id < 0)
    msg_id = msgget(IPC_KEY, 0);

  exit_on_error(msg_id, "msgget");

  Mensagem m;
  int status;
  while (1) {
    status = msgrcv(msg_id, &m, sizeof(m.consulta), 1, 0);
    exit_on_error(status, "Erro ao receber");
    printf("[Servidor] Chegou novo pedido de consulta do tipo %d, descrição %s e PID %d\n", m.consulta.tipo, m.consulta.descricao, m.consulta.pid_consulta);


    int child_pid = fork();
    exit_on_error(child_pid, "Erro na criação de servidor dedicado");
    if (child_pid == 0) {
      // Processo dedicado

      // Baixar o valor do semáforo
      // (caso o semáforo esteja a zero o processo fica em espera)
      int status = semop(sem_id, &DOWN, 1);
      exit_on_error(status, "DOWN");
      printf("DENTRO\n");

      // Verifica se a lista está cheia
      int i;
      for (i = 0; i <= 10; i++) {
        if (mem->lista_consultas[i].tipo == -1)
          break;
        if (i == 10) {
          printf("Lista de consultas cheia");
          m.tipo = m.consulta.pid_consulta;
          m.consulta.status = 4;
          status = msgsnd(IPC_EXCL, &m, sizeof(m.consulta), 0);
          exit_on_error(status, "Erro ao enviar tipo 4\n");
          mem->contadores[3]++;
          exit(0);
        }
      }

      // Encontra uma sala vazia e agenda consulta
      for (i = 0; i < 10; i++) {
        if (mem->lista_consultas[i].tipo == -1) {
          mem->lista_consultas[i] = m.consulta;
          mem->contadores[m.consulta.tipo - 1]++;
          printf("[Servidor] Consulta agendada para a sala %d\n", i);

          // Comunica com o cliente
          m.tipo = m.consulta.pid_consulta;
          m.consulta.status = 2;
          status = msgsnd(msg_id, &m, sizeof(m.consulta), 0);
          exit_on_error(status, "Erro ao enviar status 2\n");

          // Sobe o valor do semáforo
          status = semop(sem_id, &UP, 1);
          exit_on_error(status, "UP");
          printf("FORA\n");

          // Aguarda conclusão de consulta
          int count = 0;
          while (count < DURACAO) {
            sleep(1);
            printf("PID da consulta : %d\n", m.consulta.pid_consulta);
            status = msgrcv(msg_id, &m, sizeof(m.consulta), m.consulta.pid_consulta, IPC_NOWAIT);
            if (m.consulta.status == 5) {

              // Baixa o valor do semáforo
              int status = semop(sem_id, &DOWN, 1);
              exit_on_error(status, "DOWN");
              printf("DENTRO\n");

              remover_consulta(i, mem);

              // Sobe o valor do semáforo
              status = semop(sem_id, &UP, 1);
              exit_on_error(status, "UP");
              printf("FORA\n");

              printf("Consulta cancelada pelo utilizador %ld\n", m.tipo);
              exit(1);
            }
            count++;
          }

          printf("[Servidor] Consulta terminada na sala %d\n", i);
          m.consulta.status = 3;
          status = msgsnd(msg_id, &m, sizeof(m.consulta), 0);
          exit_on_error(status, "Erro ao enviar status 3\n");

          // Baixa o valor do semáforo
          int status = semop(sem_id, &DOWN, 1);
          exit_on_error(status, "DOWN");
          printf("DENTRO\n");

          // Remover consulta da lista
          printf("A remover consulta na posição %d\n", i);
          // TODO: REMOVER SLEEP
          sleep(7);
          remover_consulta(i, mem);

          // Sobe o valor do semáforo
          status = semop(sem_id, &UP, 1);
          exit_on_error(status, "UP");
          printf("FORA\n");

          exit(0);
        }
      }
    }
  }
}
