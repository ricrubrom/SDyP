
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
pthread_mutex_t mutex;
double avg_sequential = 0;
double avg = 0;

void sequential()
{
  int i;
  for (i = 0; i < N; i++)
  {
    avg_sequential += V[i];
  }
  avg_sequential /= N;
}

void *threadf(void *arg)
{
  int tid = *(int *)arg;
  int i;
  int step = N / t;
  double avg_partial = 0;
  printf("Hilo id:%d\n", tid);
  for (i = tid * step; i < (tid + 1) * step; i++)
  {
    avg_partial += V[i];
  }
  pthread_mutex_lock(&mutex);
  avg += avg_partial / N;
  pthread_mutex_unlock(&mutex);

  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

  int i;
  double timetick;
  mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

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
    V[i] = rand() % 100;
  }

  timetick = dwalltime();
  sequential();
  double sequential_time = dwalltime() - timetick;
  printf("Tiempo secuencial en segundos %f\n", sequential_time);
  printf("avg: %f\n", avg_sequential);

  for (t = 2; t <= T; t *= 2)
  {
    pthread_t mis_Threads[t];
    int threads_ids[t];
    avg = 0;

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
    if (avg_sequential == avg)
    {
      printf("Resultado correcto\n");
    }
    else
    {
      printf("Resultado erroneo\n");
    }
  }

  free(V);
  pthread_mutex_destroy(&mutex);

  return (0);
}