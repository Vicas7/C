#include "clinic-iul.h"

Consulta lista_consultas[10];

Consulta* iniciar() {
  // Cria um semáforo para sincronização
  int sem_id = semget(IPC_KEY, 1, IPC_CREAT | 0666);
  exit_on_error(sem_id, "semget");

  // Inicializa o semáforo a 0 para impedir que outro processo possa
  // aceder à zona de memória partilhada antes da inicialização terminar
  int status = semctl(sem_id, 0, SETVAL, 0);
  exit_on_error(status, "semctl(SETVAL)");

  // TODO Fazer as duas memorias. Tratar da parte do contador
  // Cria | conecta a zona de memória partilhada
  int mem_id = shmget(IPC_KEY, 10 * sizeof(Consulta), 0);
  if (mem_id < 0)
    mem_id = shmget(IPC_KEY, 10 * sizeof(Consulta), IPC_EXCL | IPC_CREAT | 0666);
  exit_on_error(mem_id, "shmget");

  // Obtém o apontador para a zona de memória partilhada
  Consulta* c = (Consulta*)shmat(mem_id, NULL, 0);
  exit_on_null(c, "shmat");

  // Inicializa os valores
  for (int i = 0; i < 10; i++) {
    c[i].tipo = -1;
  }
  printf("Memória iniciada id=%d\n", mem_id);

  // A inicialização terminou, coloca o semáforo a 1 para que outros
  // processos possam aceder à zona de memória partilhada
  status = semctl(sem_id, 0, SETVAL, 1);
  exit_on_error(status, "semctl(SETVAL)");
  return c;
}

void remover_consulta(int posicao, Consulta* c) {
  int i;
  for (i = posicao; i < 9; i++) {
    if (c[i + 1].tipo > 0) {
      c[i] = c[i + 1];
    }
    else {
      break;
    }
  }
  c[i].tipo = -1;
}

int main() {
  iniciar();
  int mem_id = shmget(IPC_KEY, 10 * sizeof(Consulta), 0);
  exit_on_error(mem_id, "shmget");

  // Obtém o apontador para a zona de memória partilhada
  Consulta* c = (Consulta*)shmat(mem_id, NULL, 0);
  exit_on_null(c, "shmat");

  // Obter identificador do semáforo
  int sem_id = semget(IPC_KEY, 1, 0);
  exit_on_error(sem_id, "semget");

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
      struct sembuf DOWN = { .sem_op = -1 };
      int status = semop(sem_id, &DOWN, 1);
      exit_on_error(status, "DOWN");

      // Verifica se a lista está cheia
      int i;
      if (c[9].tipo != -1) {
        printf("Lista de consultas cheia");
        m.tipo = m.consulta.pid_consulta;
        m.consulta.status = 4;
        status = msgsnd(IPC_EXCL, &m, sizeof(m.consulta), 0);
        exit_on_error(status, "Erro ao enviar tipo 4\n");
        exit(0);
        // TODO Incrementar contador de consultas perdidas
      }

      // Encontra uma sala vazia e agenda consulta
      for (i = 0; i < 10; i++) {
        if (c[i].tipo == -1) {
          c[i] = m.consulta;
          printf("[Servidor] Consulta agendada para a sala %d\n", i);
          // TODO Incrementear o contador do tipo correspondente 

          // Comunica com o cliente
          m.tipo = m.consulta.pid_consulta;
          m.consulta.status = 2;
          status = msgsnd(msg_id, &m, sizeof(m.consulta), 0);
          exit_on_error(status, "Erro ao enviar status 2\n");

          // Sobe o valor do semáforo
          struct sembuf UP = { .sem_op = +1 };
          status = semop(sem_id, &UP, 1);
          exit_on_error(status, "UP");

          // Aguarda conclusão de consulta
          sleep(DURACAO);

          printf("[Servidor] Consulta terminada na sala %d\n", i);
          m.consulta.status = 3;
          status = msgsnd(msg_id, &m, sizeof(m.consulta), 0);
          exit_on_error(status, "Erro ao enviar status 3\n");

          // Baixa o valor do semáforo
          struct sembuf DOWN = { .sem_op = -1 };
          int status = semop(sem_id, &DOWN, 1);
          exit_on_error(status, "DOWN");

          // Remover consulta da lista
          remover_consulta(i, c);

          // Sobe o valor do semáforo
          struct sembuf UP = { .sem_op = +1 };
          status = semop(sem_id, &UP, 1);
          exit_on_error(status, "UP");

          exit(0);
        }
      }
    }




  }
}
