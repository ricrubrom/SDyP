
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../utils/utils.h"
#include <pthread.h>

// Global
unsigned long N = 100;
int T = 1;
int t = 1;
int *V;
int *sorted_sequential;
int *sorted_parallel;
pthread_barrier_t barrier;

void merge(int arr[], int l, int m, int r)
{ // Barrera necesito
  int i, j, k;
  int n1 = m - l + 1;
  int n2 = r - m;

  // Create temp arrays
  int L[n1], R[n2];

  // Copy data to temp arrays L[] and R[]
  for (i = 0; i < n1; i++)
    L[i] = arr[l + i];
  for (j = 0; j < n2; j++)
    R[j] = arr[m + 1 + j];

  // Merge the temp arrays back into arr[l..r
  i = 0;
  j = 0;
  k = l;
  while (i < n1 && j < n2)
  {
    if (L[i] <= R[j])
    {
      arr[k] = L[i];
      i++;
    }
    else
    {
      arr[k] = R[j];
      j++;
    }
    k++;
  }

  // Copy the remaining elements of L[],
  // if there are any
  while (i < n1)
  {
    arr[k] = L[i];
    i++;
    k++;
  }

  // Copy the remaining elements of R[],
  // if there are any
  while (j < n2)
  {
    arr[k] = R[j];
    j++;
    k++;
  }
}

// l is for left index and r is right index of the
// sub-array of arr to be sorted
void mergeSort(int arr[], int l, int r)
{
  if (l < r)
  {
    int m = l + (r - l) / 2;

    // Sort first and second halves
    mergeSort(arr, l, m);
    mergeSort(arr, m + 1, r);

    merge(arr, l, m, r);
  }
}

void sequential()
{
  mergeSort(sorted_sequential, 0, N - 1);
}

void *threadf(void *arg)
{
  int tid = *(int *)arg;
  int step = N / t;
  int left = tid * step;
  int right = (tid + 1) * step - 1;

  printf("Hilo id:%d\n", tid);
  mergeSort(sorted_parallel, left, right);
  pthread_barrier_wait(&barrier); // Synchronize threads before merging

  for (int i = 2; i <= t; i *= 2)
  {
    if (tid % i == 0)
    {
      int mid = left + (step * i / 2) - 1;
      int merge_right = left + (step * i) - 1;
      if (merge_right < N) // Ensure bounds
        merge(sorted_parallel, left, mid, merge_right);
    }
    pthread_barrier_wait(&barrier); // Synchronize before next merge step
  }

  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

  int i;
  double timetick;
  char check = 1;

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

  V = (int *)malloc(sizeof(int) * N);
  sorted_sequential = (int *)malloc(sizeof(int) * N);
  sorted_parallel = (int *)malloc(sizeof(int) * N);

  srand(time(NULL));

  for (i = 0; i < N; i++)
  {
    V[i] = rand() % 100;
    sorted_sequential[i] = V[i];
    sorted_parallel[i] = V[i];
  }

  timetick = dwalltime();
  sequential();
  double sequential_time = dwalltime() - timetick;
  printf("Tiempo secuencial en segundos %f\n", sequential_time);
  for (i = 0; i < N; i++)
  {
    // printf("%d ", sorted_sequential[i]);
  }

  for (t = 2; t <= T; t *= 2)
  {
    pthread_t mis_Threads[t];
    int threads_ids[t];
    pthread_barrier_init(&barrier, NULL, t);

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
    for (i = 0; i < N; i++)
    {
      // printf("%d ", sorted_parallel[i]);
    }
    for (i = 0; i < N; i++)
    {
      if (sorted_sequential[i] != sorted_parallel[i])
      {
        check = 0;
      }
    }
    if (check)
    {
      printf("Resultado correcto\n");
    }
    else
    {
      printf("Resultado erroneo\n");
    }
    pthread_barrier_destroy(&barrier);
  }

  free(V);
  free(sorted_sequential);
  free(sorted_parallel);

  return (0);
}