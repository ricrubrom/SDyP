#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MASTER 0

void merge(int *arr, int l, int m, int r)
{
  int n1 = m - l + 1;
  int n2 = r - m;
  int *L = malloc(n1 * sizeof(int));
  int *R = malloc(n2 * sizeof(int));

  for (int i = 0; i < n1; i++)
    L[i] = arr[l + i];
  for (int j = 0; j < n2; j++)
    R[j] = arr[m + 1 + j];

  int i = 0, j = 0, k = l;
  while (i < n1 && j < n2)
  {
    arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
  }
  while (i < n1)
    arr[k++] = L[i++];
  while (j < n2)
    arr[k++] = R[j++];

  free(L);
  free(R);
}

void merge_sort(int *arr, int l, int r)
{
  if (l < r)
  {
    int m = l + (r - l) / 2;
    merge_sort(arr, l, m);
    merge_sort(arr, m + 1, r);
    merge(arr, l, m, r);
  }
}

void proceso(int id, int P, int N)
{
  int *array = NULL;
  int *local_array = NULL;
  int local_n = N / P;
  double tiempo_paralelo = 0.0, tiempo_secuencial = 0.0;
  double inicio, fin;

  if (id == MASTER)
  {
    array = malloc(N * sizeof(int));
    srand(42);
    for (int i = 0; i < N; i++)
      array[i] = rand() % 10000;
  }

  local_array = malloc(local_n * sizeof(int));

  MPI_Barrier(MPI_COMM_WORLD);
  inicio = MPI_Wtime();

  MPI_Scatter(array, local_n, MPI_INT, local_array, local_n, MPI_INT, MASTER, MPI_COMM_WORLD);

  merge_sort(local_array, 0, local_n - 1);

  // Fase de combinación
  int step = 1;
  while (step < P)
  {
    if (id % (2 * step) == 0)
    {
      if (id + step < P)
      {
        int recv_size;
        MPI_Recv(&recv_size, 1, MPI_INT, id + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int *recv_array = malloc(recv_size * sizeof(int));
        MPI_Recv(recv_array, recv_size, MPI_INT, id + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int *merged_array = malloc((local_n + recv_size) * sizeof(int));
        int i = 0, j = 0, k = 0;
        while (i < local_n && j < recv_size)
        {
          if (local_array[i] <= recv_array[j])
            merged_array[k++] = local_array[i++];
          else
            merged_array[k++] = recv_array[j++];
        }
        while (i < local_n)
          merged_array[k++] = local_array[i++];
        while (j < recv_size)
          merged_array[k++] = recv_array[j++];

        free(local_array);
        free(recv_array);
        local_array = merged_array;
        local_n += recv_size;
      }
    }
    else
    {
      int destino = id - step;
      MPI_Send(&local_n, 1, MPI_INT, destino, 0, MPI_COMM_WORLD);
      MPI_Send(local_array, local_n, MPI_INT, destino, 0, MPI_COMM_WORLD);
      free(local_array);
      break;
    }
    step *= 2;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  fin = MPI_Wtime();
  tiempo_paralelo = fin - inicio;

  if (id == MASTER)
  {

    // Ahora el master ejecuta secuencial
    srand(42); // regenerar mismo arreglo
    for (int i = 0; i < N; i++)
      array[i] = rand() % 10000;

    inicio = MPI_Wtime();
    merge_sort(array, 0, N - 1);
    fin = MPI_Wtime();
    tiempo_secuencial = fin - inicio;

    printf("Tiempo de ejecución secuencial: %.6f segundos\n", tiempo_secuencial);
    printf("Tiempo de ejecución paralelo: %.6f segundos\n", tiempo_paralelo);

    double speedup = tiempo_secuencial / tiempo_paralelo;
    double eficiencia = (speedup / P) * 100.0;

    printf("Speedup: %.2f\n", speedup);
    printf("Eficiencia: %.2f%%\n", eficiencia);

    free(array);
  }
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int id, P;
  long long unsigned int N;
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Comm_size(MPI_COMM_WORLD, &P);

  if (argc < 2)
  {
    if (id == 0)
      printf("Uso: %s <N>\n", argv[0]);
    MPI_Finalize();
    return 1;
  }

  N = atoi(argv[1]);

  if (N % P != 0)
  {
    if (id == 0)
      printf("Error: N debe ser divisible por la cantidad de procesos\n");
    MPI_Finalize();
    return 1;
  }

  proceso(id, P, N);

  MPI_Finalize();
  return 0;
}
