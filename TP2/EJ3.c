
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
pthread_mutex_t mutex_min;
pthread_mutex_t mutex_max;
int min_sequential = 256;
int max_sequential = -1;
int min = 256;
int max = -1;

void sequential()
{
  int i;
  for (i = 0; i < N; i++)
  {
    if (V[i] < min_sequential)
    {
      min_sequential = V[i];
    }
    if (V[i] > max_sequential)
    {
      max_sequential = V[i];
    }
  }
}

void *threadf(void *arg)
{
  int tid = *(int *)arg;
  int i;
  int step = N / t;
  int min_parcial = 256;
  int max_parcial = -1;
  printf("Hilo id:%d\n", tid);
  for (i = tid * step; i < (tid + 1) * step; i++)
  {
    if (V[i] < min)
      if (V[i] < min_parcial)
      {
        min_parcial = V[i];
      }
    if (V[i] > max_parcial)
    {
      max_parcial = V[i];
    }
  }
  pthread_mutex_lock(&mutex_min);
  if (min_parcial < min)
  {
    min = min_parcial;
  }
  pthread_mutex_unlock(&mutex_min);
  pthread_mutex_lock(&mutex_max);
  if (max_parcial > max)
  {
    max = max_parcial;
  }
  pthread_mutex_unlock(&mutex_max);
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

  int i;
  double timetick;
  mutex_min = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  mutex_max = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

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
    V[i] = rand() % 1000;
  }

  timetick = dwalltime();
  sequential();
  double sequential_time = dwalltime() - timetick;
  printf("Tiempo secuencial en segundos %f\n", sequential_time);
  printf("min: %d, max: %d\n", min_sequential, max_sequential);

  for (t = 2; t <= T; t *= 2)
  {
    pthread_t mis_Threads[t];
    int threads_ids[t];
    min = 256;
    max = -1;

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
    if ((min_sequential == min) && (max_sequential == max))
    {
      printf("Resultado correcto\n");
    }
    else
    {
      printf("Resultado erroneo\n");
    }
  }

  free(V);
  pthread_mutex_destroy(&mutex_min);
  pthread_mutex_destroy(&mutex_max);
  return (0);
}