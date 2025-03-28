// Izquierda Filas, Derecha Columnas
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../utils/utils.h"
#include <pthread.h>

// Global
int N = 100;
int T = 1;
int t = 0;
double *A, *B, *C;

void sequential()
{
  int i, j, k;

  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      C[i + j * N] = 0;
      for (k = 0; k < N; k++)
      {

        C[i + j * N] = C[i + j * N] + A[i * N + k] * B[k + j * N];
      }
    }
  }
}

void *threadf(void *arg)
{
  int tid = *(int *)arg;
  int i, j, k;
  int step = N / t;
  printf("Hilo id:%d\n", tid);
  for (i = tid * step; i < (tid + 1) * step; i++)
  {
    for (j = 0; j < N; j++)
    {
      C[i + j * N] = 0;
      for (k = 0; k < N; k++)
      {

        C[i + j * N] = C[i + j * N] + A[i * N + k] * B[k + j * N];
      }
    }
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  // Adentro del main
  int check = 1;
  int i, j, k;
  double timetick;

  // Controla los argumentos al programa
  if (argc != 3)
  {
    printf("\nUsar: %s n t\n  n: Dimension de la matriz (nxn X nxn)\n  t: Cantidad de hilos\n", argv[0]);
    exit(1);
  }

  N = atoi(argv[1]);
  T = atoi(argv[2]);

  if (N <= 0 || T <= 0)
  {
    printf("\nError: N y T deben ser mayores que 0.\n");
    exit(1);
  }

  A = (double *)malloc(sizeof(double) * N * N);
  B = (double *)malloc(sizeof(double) * N * N);
  C = (double *)malloc(sizeof(double) * N * N);
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      A[i * N + j] = 1;
      B[i + j * N] = 1;
    }
  }

  timetick = dwalltime();
  sequential();
  double sequential_time = dwalltime() - timetick;
  printf("Tiempo secuencial en segundos %f\n", sequential_time);

  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      check = check && (C[i + j * N] == N);
    }
  }

  if (check)
  {
    printf("Multiplicacion de matrices resultado correcto\n");
  }
  else
  {
    printf("Multiplicacion de matrices resultado erroneo\n");
  }

  for (t = 2; t <= T; t *= 2)
  {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    pthread_t mis_Threads[t];
    int threads_ids[t];

    timetick = dwalltime();

    for (int id = 0; id < t; id++)
    {
      threads_ids[id] = id;
      pthread_create(&mis_Threads[id], NULL, &threadf, (void *)&threads_ids[id]);
    }

    for (int id = 0; id < t; id++)
    {
      pthread_join(mis_Threads[id], NULL);
    }
    double parallel_time = dwalltime() - timetick;
    printf("Tiempo en segundos %f\n", dwalltime() - timetick);
    printf("Speedup: %f\n", sequential_time / parallel_time);
    printf("Eficiencia: %f\n", (sequential_time / parallel_time) / t);

    // Verifica el resultado
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        check = check && (C[i + j * N] == N);
      }
    }

    if (check)
    {
      printf("Multiplicacion de matrices resultado correcto\n");
    }
    else
    {
      printf("Multiplicacion de matrices resultado erroneo\n");
    }
  }

  free(A);
  free(B);
  free(C);
  return (0);
}