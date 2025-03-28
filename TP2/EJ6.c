
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../utils/utils.h"
#include <pthread.h>

// Global
unsigned long N = 100;
int T = 1;
int t = 1;
double *V;
char monotonic_sequential = 1;
char monotonic = 1;

void sequential()
{
  int i;
  int prev = V[0];
  if (prev <= V[1])
  {
    prev = V[1];
    for (i = 2; i < N; i++)
    {
      if (prev <= V[i])
      {
        prev = V[i];
      }
      else
      {
        monotonic_sequential = 0;
      }
    }
  }
  else
  {
    prev = V[1];
    for (i = 2; i < N; i++)
    {
      if (prev >= V[i])
      {
        prev = V[i];
      }
      else
      {
        monotonic_sequential = 0;
      }
    }
  }
}

void *threadf(void *arg)
{
  int tid = *(int *)arg;
  int i;
  int step = N / t;
  printf("Hilo id:%d\n", tid);
  int prev = V[tid * step];
  if (prev <= V[tid * step + 1])
  {
    prev = V[tid * step + 1];
    for (i = tid * step + 2; i < (tid + 1) * step; i++)
    {
      if (prev <= V[i])
      {
        prev = V[i];
      }
      else
      {
        monotonic = 0;
      }
    }
  }
  else
  {
    prev = V[tid * step + 1];
    for (i = tid * step + 2; i < (tid + 1) * step; i++)
    {
      if (prev >= V[i])
      {
        prev = V[i];
      }
      else
      {
        monotonic = 0;
      }
    }
  }

  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

  int i;
  double timetick;

  // Controla los argumentos al programa
  if (argc != 3)
  {
    printf("\nUsar: %s n t\n  n: Dimension de la matriz (nxn X nxn)\n  t: Cantidad de hilos\n", argv[0]);
    exit(1);
  }

  N = 1 << atoi(argv[1]);
  T = atoi(argv[2]);

  if (N <= 1 || T <= 0)
  {
    printf("\nError: N y T deben ser mayores que 0.\n");
    exit(1);
  }

  V = (double *)malloc(sizeof(double) * N);

  srand(time(NULL));

  for (i = 0; i < N; i++)
  {
    // V[i] = rand() % 100;
    V[i] = i;
  }

  timetick = dwalltime();
  sequential();
  double sequential_time = dwalltime() - timetick;
  printf("Tiempo secuencial en segundos %f\n", sequential_time);
  printf("Monotonic: %d\n", monotonic_sequential);
  for (t = 2; t <= T; t *= 2)
  {
    pthread_t mis_Threads[t];
    int threads_ids[t];
    monotonic = 1;

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
    printf("Tiempo en segundos %f\n", parallel_time);
    printf("Speedup: %f\n", sequential_time / parallel_time);
    printf("Eficencia: %f\n", (sequential_time / parallel_time) / t);
    if (monotonic_sequential == monotonic)
    {
      printf("Resultado correcto\n");
    }
    else
    {
      printf("Resultado erroneo\n");
    }
  }

  free(V);

  return (0);
}