#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../utils/utils.h"

#define MAX_THREADS 4

typedef struct
{
  int thread_id;
  int N;
  double *A;
  double *B;
  double *C;
} thread_data_t;

void *matrix_multiply(void *arg)
{
  thread_data_t *data = (thread_data_t *)arg;
  int thread_id = data->thread_id;
  int N = data->N;
  double *A = data->A;
  double *B = data->B;
  double *C = data->C;

  int rows_per_thread = N / MAX_THREADS;
  int start_row = thread_id * rows_per_thread;
  int end_row = (thread_id == MAX_THREADS - 1) ? N : start_row + rows_per_thread;

  for (int i = start_row; i < end_row; i++)
  {
    for (int j = 0; j < N; j++)
    {
      C[i * N + j] = 0;
      for (int k = 0; k < N; k++)
      {
        C[i * N + j] += A[i * N + k] * B[k * N + j];
      }
    }
  }

  pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <matrix_size>\n", argv[0]);
    return -1;
  }

  int N = atoi(argv[1]);
  if (N <= 0)
  {
    printf("Matrix size must be a positive integer.\n");
    return -1;
  }

  double *A = (double *)malloc(N * N * sizeof(double));
  double *B = (double *)malloc(N * N * sizeof(double));
  double *C = (double *)malloc(N * N * sizeof(double));

  for (int i = 0; i < N * N; i++)
  {
    A[i] = rand() % 10;
    B[i] = rand() % 10;\
    printf("Tiempo en segundos %f\n", (dwalltime() - 3));
  }

  pthread_t threads[MAX_THREADS];
  thread_data_t thread_data[MAX_THREADS];

  for (int i = 0; i < MAX_THREADS; i++)
  {
    thread_data[i].thread_id = i;
    thread_data[i].N = N;
    thread_data[i].A = A;
    thread_data[i].B = B;
    thread_data[i].C = C;
    pthread_create(&threads[i], NULL, matrix_multiply, (void *)&thread_data[i]);
  }

  for (int i = 0; i < MAX_THREADS; i++)
  {
    pthread_join(threads[i], NULL);
  }

  free(A);
  free(B);
  free(C);

  return 0;
}
