#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../utils/utils.h"

void print_array(int *arr, int size)
{
  for (int i = 0; i < size; i++)
    printf("%d ", arr[i]);
  printf("\n");
}

void merge(int *arr, int left, int middle, int right)
{
  int i, j, k;
  int n1 = middle - left + 1;
  int n2 = right - middle;

  // Create temporary arrays with proper size checks
  int *arr1 = (int *)malloc(sizeof(int) * n1);
  int *arr2 = (int *)malloc(sizeof(int) * n2);

  if (arr1 == NULL || arr2 == NULL)
  {
    fprintf(stderr, "Error: Memory allocation failed in merge\n");
    if (arr1)
      free(arr1);
    if (arr2)
      free(arr2);
    return;
  }

  for (i = 0; i < n1; i++)
    arr1[i] = arr[left + i];
  for (i = 0; i < n2; i++)
    arr2[i] = arr[middle + 1 + i];

  i = 0;
  j = 0;
  k = left;

  while (i < n1 && j < n2)
  {
    if (arr1[i] <= arr2[j])
    {
      arr[k] = arr1[i];
      i++;
    }
    else
    {
      arr[k] = arr2[j];
      j++;
    }
    k++;
  }

  while (i < n1)
  {
    arr[k] = arr1[i];
    i++;
    k++;
  }

  while (j < n2)
  {
    arr[k] = arr2[j];
    j++;
    k++;
  }

  free(arr1);
  free(arr2);
}

void mergesort(int *arr, int left, int right)
{
  if (left < right)
  {
    int middle = left + (right - left) / 2;

    mergesort(arr, left, middle);
    mergesort(arr, middle + 1, right);

    merge(arr, left, middle, right);
  }
}

int is_sorted(int *arr, int size)
{
  for (int i = 1; i < size; i++)
  {
    if (arr[i] < arr[i - 1])
      return 0;
  }
  return 1;
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int id, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  if (argc < 2)
  {
    if (id == 0)
      printf("Uso: %s <N>\n", argv[0]);
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  long unsigned int N = atoi(argv[1]);

  if (N % comm_size != 0)
  {
    if (id == 0)
      printf("Error: N debe ser múltiplo de la cantidad de procesos (%d).\n", comm_size);
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  int work_load = N / comm_size;

  int *A = NULL;
  int *local_a = NULL;

  // Allocate memory with proper error handling
  local_a = (int *)malloc(sizeof(int) * work_load);
  if (local_a == NULL)
  {
    fprintf(stderr, "Error: Memory allocation failed for local_a\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  double start, end;
  double sequential_time = 0;
  double parallel_time;

  if (id == 0)
  {
    // Allocate memory with proper error handling
    A = (int *)malloc(sizeof(int) * N);
    if (A == NULL)
    {
      fprintf(stderr, "Error: Memory allocation failed for array A\n");
      free(local_a);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
      return EXIT_FAILURE;
    }

    srand(34);
    for (int i = 0; i < N; i++)
    {
      A[i] = rand() % 100;
    }

    // Make a copy for verification
    int *A_copy = (int *)malloc(sizeof(int) * N);
    if (A_copy == NULL)
    {
      fprintf(stderr, "Error: Memory allocation failed for A_copy\n");
      free(A);
      free(local_a);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
      return EXIT_FAILURE;
    }

    for (int i = 0; i < N; i++)
    {
      A_copy[i] = A[i];
    }
    // Sequential execution for benchmarking
    start = MPI_Wtime();

    mergesort(A_copy, 0, N - 1);

    end = MPI_Wtime();
    sequential_time = end - start;
    printf("Tiempo de ejecución secuencial: %f segundos\n", sequential_time);

    if (is_sorted(A_copy, N))
      printf("El arreglo está ordenado correctamente.\n");
    else
      printf("El arreglo NO está ordenado correctamente.\n");

    free(A_copy);

    // Reset random seed for parallel test
    srand(34);
    for (int i = 0; i < N; i++)
    {
      A[i] = rand() % 100;
    }
  }

  // Start parallel timing
  MPI_Barrier(MPI_COMM_WORLD);
  start = MPI_Wtime();

  // Distribute data
  MPI_Scatter(A, work_load, MPI_INT, local_a, work_load, MPI_INT, 0, MPI_COMM_WORLD);

  // Local sort
  mergesort(local_a, 0, work_load - 1);

  int current_size = work_load;
  int *temp_buffer = NULL;
  int step = 1;

  // Bottom-up merge tree
  while (step < comm_size)
  {
    if (id % (2 * step) == 0)
    {
      int partner = id + step;

      // Only receive if partner exists
      if (partner < comm_size)
      {
        int recv_size;
        MPI_Status status;

        // Get size first
        MPI_Recv(&recv_size, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, &status);

        // Allocate memory for received data
        temp_buffer = (int *)malloc(sizeof(int) * recv_size);
        if (temp_buffer == NULL)
        {
          fprintf(stderr, "Error: Memory allocation failed for temp_buffer\n");
          free(local_a);
          MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
          return EXIT_FAILURE;
        }

        // Receive data
        MPI_Recv(temp_buffer, recv_size, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);

        // Allocate memory for merged result
        int *merged = (int *)malloc(sizeof(int) * (current_size + recv_size));
        if (merged == NULL)
        {
          fprintf(stderr, "Error: Memory allocation failed for merged array\n");
          free(temp_buffer);
          free(local_a);
          MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
          return EXIT_FAILURE;
        }

        // Copy both arrays to the merged array
        int middle = current_size - 1;
        for (int i = 0; i < current_size; i++)
          merged[i] = local_a[i];
        for (int i = 0; i < recv_size; i++)
          merged[current_size + i] = temp_buffer[i];

        // Merge the two sorted subarrays
        merge(merged, 0, middle, current_size + recv_size - 1);

        // Update local array
        free(local_a);
        free(temp_buffer);
        local_a = merged;
        current_size += recv_size;
      }
    }
    else
    {
      // Send data to partner
      int dest = id - step;

      // First send size
      MPI_Send(&current_size, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);

      // Then send data
      MPI_Send(local_a, current_size, MPI_INT, dest, 1, MPI_COMM_WORLD);

      // This process no longer participates
      free(local_a);
      local_a = NULL;
      break;
    }

    step *= 2;
  }

  // End timing
  MPI_Barrier(MPI_COMM_WORLD);
  end = MPI_Wtime();
  parallel_time = end - start;

  // Print results
  if (id == 0)
  {
    printf("Tiempo de ejecución paralelo: %f segundos\n", parallel_time);

    if (is_sorted(local_a, N))
      printf("El arreglo paralelo está ordenado correctamente.\n");
    else
      printf("El arreglo paralelo NO está ordenado correctamente.\n");

    double speedup = sequential_time / parallel_time;
    double efficiency = (speedup / comm_size) * 100;

    printf("Speedup: %f\n", speedup);
    printf("Eficiencia: %f\n", efficiency);

    free(A);
  }

  if (local_a != NULL)
    free(local_a);

  MPI_Finalize();
  return EXIT_SUCCESS;
}