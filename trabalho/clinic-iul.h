#ifndef __CLINIC_IUL_H__
#define __CLINIC_IUL_H__

typedef struct
{
  int tipo;            // Tipo de Consulta: 1-Normal, 2-COVID19, 3-Urgente
  char descricao[100]; // Descrição da Consulta
  int pid_consulta;    // PID do processo que quer fazer a consulta
} Consulta;

#endif